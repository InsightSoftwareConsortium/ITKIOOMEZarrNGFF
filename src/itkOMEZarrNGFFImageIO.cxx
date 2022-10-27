/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkOMEZarrNGFFImageIO.h"
#include "itkIOCommon.h"
#include "itkIntTypes.h"
#include "itkByteSwapper.h"

#include "tensorstore/context.h"
#include "tensorstore/open.h"
#include "tensorstore/index_space/dim_expression.h"

namespace itk
{
static tensorstore::Context tsContext = tensorstore::Context::Default();

OMEZarrNGFFImageIO::OMEZarrNGFFImageIO()
{
  this->AddSupportedWriteExtension(".zarr");
  this->AddSupportedWriteExtension(".zr2");
  this->AddSupportedWriteExtension(".zr3");
  this->AddSupportedWriteExtension(".zip");

  this->AddSupportedReadExtension(".zarr");
  this->AddSupportedReadExtension(".zr2");
  this->AddSupportedReadExtension(".zr3");
  this->AddSupportedReadExtension(".zip");

  this->Self::SetCompressor("");
  this->Self::SetMaximumCompressionLevel(9);
  this->Self::SetCompressionLevel(2);
}


void
OMEZarrNGFFImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent); // TODO: either add stuff here, or remove this override
}

IOComponentEnum
tensorstoreToITKComponentType(const tensorstore::DataType dtype)
{
  switch (dtype.id())
  {
    case tensorstore::DataTypeId::char_t:
    case tensorstore::DataTypeId::int8_t:
      return IOComponentEnum::CHAR;

    case tensorstore::DataTypeId::byte_t:
    case tensorstore::DataTypeId::uint8_t:
      return IOComponentEnum::UCHAR;

    case tensorstore::DataTypeId::int16_t:
      return IOComponentEnum::SHORT;

    case tensorstore::DataTypeId::uint16_t:
      return IOComponentEnum::USHORT;

    case tensorstore::DataTypeId::int32_t:
      return IOComponentEnum::INT;

    case tensorstore::DataTypeId::uint32_t:
      return IOComponentEnum::UINT;

    case tensorstore::DataTypeId::int64_t:
      return IOComponentEnum::LONGLONG;

    case tensorstore::DataTypeId::uint64_t:
      return IOComponentEnum::ULONGLONG;

    case tensorstore::DataTypeId::float32_t:
      return IOComponentEnum::FLOAT;

    case tensorstore::DataTypeId::float64_t:
      return IOComponentEnum::DOUBLE;

    default:
      return IOComponentEnum::UNKNOWNCOMPONENTTYPE;
  }
}

tensorstore::DataType
itkToTensorstoreComponentType(const IOComponentEnum itkComponentType)
{
  switch (itkComponentType)
  {
    case IOComponentEnum::UNKNOWNCOMPONENTTYPE:
      return tensorstore::dtype_v<void>;

    case IOComponentEnum::CHAR:
      return tensorstore::dtype_v<int8_t>;

    case IOComponentEnum::UCHAR:
      return tensorstore::dtype_v<uint8_t>;

    case IOComponentEnum::SHORT:
      return tensorstore::dtype_v<int16_t>;

    case IOComponentEnum::USHORT:
      return tensorstore::dtype_v<uint16_t>;

    // "long" is a silly type because it basically guaranteed not to be
    // cross-platform across 32-vs-64 bit machines, but we can figure out
    // a cross-platform way of storing the information.
    case IOComponentEnum::LONG:
      if (4 == sizeof(long))
      {
        return tensorstore::dtype_v<int32_t>;
      }
      else
      {
        return tensorstore::dtype_v<int64_t>;
      }

    case IOComponentEnum::ULONG:
      if (4 == sizeof(long))
      {
        return tensorstore::dtype_v<uint32_t>;
      }
      else
      {
        return tensorstore::dtype_v<uint64_t>;
      }

    case IOComponentEnum::INT:
      return tensorstore::dtype_v<int32_t>;

    case IOComponentEnum::UINT:
      return tensorstore::dtype_v<uint32_t>;

    case IOComponentEnum::LONGLONG:
      return tensorstore::dtype_v<int64_t>;

    case IOComponentEnum::ULONGLONG:
      return tensorstore::dtype_v<uint64_t>;

    case IOComponentEnum::FLOAT:
      return tensorstore::dtype_v<float>;

    case IOComponentEnum::DOUBLE:
      return tensorstore::dtype_v<double>;

    case IOComponentEnum::LDOUBLE:
      return tensorstore::dtype_v<void>;

    default:
      return tensorstore::dtype_v<void>;
  }
}

bool
OMEZarrNGFFImageIO::CanReadFile(const char * filename)
{
  return this->HasSupportedWriteExtension(filename, true);
}

// Evaluate tensorstore future (statement) and error-check the result.
#define TS_EVAL_CHECK(statement)                                   \
  {                                                                \
    auto result = statement.result();                              \
    if (!result.ok()) /* error */                                  \
    {                                                              \
      itkExceptionMacro("tensorstore error: " << result.status()); \
    }                                                              \
  }                                                                \
  ITK_NOOP_STATEMENT


thread_local tensorstore::TensorStore<> store; // initialized by ReadImageInformation

void
OMEZarrNGFFImageIO::ReadImageInformation()
{
  auto openFuture =
    tensorstore::Open({ { "driver", "zarr" }, { "kvstore", { { "driver", "file" }, { "path", this->m_FileName } } } },
                      tsContext,
                      tensorstore::OpenMode::open,
                      tensorstore::RecheckCached{ false },
                      tensorstore::ReadWriteMode::read);
  TS_EVAL_CHECK(openFuture);
  store = openFuture.value();
  auto shape_span = store.domain().shape();

  tensorstore::DataType dtype = store.dtype();
  this->SetComponentType(tensorstoreToITKComponentType(dtype));

  std::vector<int64_t> dims(shape_span.rbegin(), shape_span.rend()); // convert KJI into IJK
  this->SetNumberOfDimensions(dims.size());
  for (unsigned d = 0; d < dims.size(); ++d)
  {
    this->SetDimensions(d, dims[d]);
  }
}

// We call tensorstoreToITKComponentType for each type.
// Hopefully compiler will optimize it away via constant propagation and inlining.
#define ELEMENT_READ(typeName)                                                                        \
  else if (tensorstoreToITKComponentType(tensorstore::dtype_v<typeName>) == this->GetComponentType()) \
  {                                                                                                   \
    auto * p = reinterpret_cast<typeName *>(buffer);                                                  \
    auto   arr = tensorstore::Array(p, store.domain().shape(), tensorstore::c_order);                 \
    tensorstore::Read(store, tensorstore::UnownedToShared(arr)).value();                              \
  }

void
OMEZarrNGFFImageIO::Read(void * buffer)
{
  // This comparison needs to be done carefully, we can compare 3D and 6D regions
  if (this->GetLargestRegion().GetNumberOfPixels() != m_IORegion.GetNumberOfPixels())
  {
    // Stream the data in chunks
    itkExceptionMacro(<< "Streamed reading is not yet implemented");
  }
  else
  {
    if (false) // start with a plain "if"
    {}         // so element statements can all be "else if"
    ELEMENT_READ(int8_t)
    ELEMENT_READ(uint8_t)
    ELEMENT_READ(int16_t)
    ELEMENT_READ(uint16_t)
    ELEMENT_READ(int32_t)
    ELEMENT_READ(uint32_t)
    ELEMENT_READ(int64_t)
    ELEMENT_READ(uint64_t)
    ELEMENT_READ(float)
    ELEMENT_READ(double)
    else { itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType())); }
  }
}


bool
OMEZarrNGFFImageIO::CanWriteFile(const char * name)
{
  return this->HasSupportedWriteExtension(name, true);
}


void
OMEZarrNGFFImageIO::WriteImageInformation()
{
  // we will do everything in Write() method
}


// We need to specify dtype for opening. As dtype is dependent on component type, this macro is long.
#define ELEMENT_WRITE(typeName)                                                                       \
  else if (tensorstoreToITKComponentType(tensorstore::dtype_v<typeName>) == this->GetComponentType()) \
  {                                                                                                   \
    if (sizeof(typeName) == 1)                                                                        \
    {                                                                                                 \
      dtype = "|";                                                                                    \
    }                                                                                                 \
    if (std::numeric_limits<typeName>::is_integer)                                                    \
    {                                                                                                 \
      if (std::numeric_limits<typeName>::is_signed)                                                   \
      {                                                                                               \
        dtype += 'i';                                                                                 \
      }                                                                                               \
      else                                                                                            \
      {                                                                                               \
        dtype += 'u';                                                                                 \
      }                                                                                               \
    }                                                                                                 \
    else                                                                                              \
    {                                                                                                 \
      dtype += 'f';                                                                                   \
    }                                                                                                 \
    dtype += std::to_string(sizeof(typeName));                                                        \
                                                                                                      \
    auto openFuture = tensorstore::Open(                                                              \
      {                                                                                               \
        { "driver", "zarr" },                                                                         \
        { "kvstore", { { "driver", "file" }, { "path", this->m_FileName } } },                        \
        { "metadata",                                                                                 \
          {                                                                                           \
            { "compressor", { { "id", "blosc" } } },                                                  \
            { "dtype", dtype },                                                                       \
            { "shape", shape },                                                                       \
          } },                                                                                        \
      },                                                                                              \
      tsContext,                                                                                      \
      tensorstore::OpenMode::create | tensorstore::OpenMode::delete_existing,                         \
      tensorstore::ReadWriteMode::read_write);                                                        \
    TS_EVAL_CHECK(openFuture);                                                                        \
                                                                                                      \
    auto   writeStore = openFuture.value();                                                           \
    auto * p = reinterpret_cast<typeName const *>(buffer);                                            \
    auto   arr = tensorstore::Array(p, shape, tensorstore::c_order);                                  \
    auto   writeFuture = tensorstore::Write(tensorstore::UnownedToShared(arr), writeStore);           \
    TS_EVAL_CHECK(writeFuture);                                                                       \
  }

void
OMEZarrNGFFImageIO::Write(const void * buffer)
{
  if (itkToTensorstoreComponentType(this->GetComponentType()) == tensorstore::dtype_v<void>)
  {
    itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType()));
  }

  std::vector<int64_t> shape(this->GetNumberOfDimensions());
  for (unsigned d = 0; d < shape.size(); ++d)
  {
    auto dSize = this->GetDimensions(d);
    if (dSize > std::numeric_limits<int64_t>::max())
    {
      itkExceptionMacro("This image IO uses a signed type for sizes, and "
                        << dSize << " exceeds maximum allowed size of " << std::numeric_limits<int64_t>::max());
    }
    shape[shape.size() - 1 - d] = dSize; // convert IJK into KJI
  }

  std::string dtype;
  // we prefer to write using our own endianness, so no conversion is necessary
  if (ByteSwapper<int>::SystemIsBigEndian())
  {
    dtype = ">";
  }
  else
  {
    dtype = "<";
  }

  if (false) // start with a plain "if"
  {}         // so element statements can all be "else if"
  ELEMENT_WRITE(int8_t)
  ELEMENT_WRITE(uint8_t)
  ELEMENT_WRITE(int16_t)
  ELEMENT_WRITE(uint16_t)
  ELEMENT_WRITE(int32_t)
  ELEMENT_WRITE(uint32_t)
  ELEMENT_WRITE(int64_t)
  ELEMENT_WRITE(uint64_t)
  ELEMENT_WRITE(float)
  ELEMENT_WRITE(double)
  else { itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType())); }
}

} // end namespace itk
