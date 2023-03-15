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

#include <filesystem>

#include "tensorstore/context.h"
#include "tensorstore/open.h"
#include "tensorstore/index_space/dim_expression.h"

#include <nlohmann/json.hpp>

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
  Superclass::PrintSelf(os, indent);
  os << indent << "DatasetIndex: " << m_DatasetIndex << std::endl;
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

// Returns TensorStore KvStore driver name appropriate for this path.
// Options are file, zip. TODO: http, gcs (GoogleCouldStorage), etc.
std::string
getKVstoreDriver(std::string path)
{
  if (path.size() < 4)
  {
    return "file";
  }
  if (path.substr(path.size() - 4) == ".zip")
  {
    return "zip";
  }
  return "file";
}

// JSON file path, e.g. "C:/Dev/ITKIOOMEZarrNGFF/v0.4/cyx.ome.zarr/.zgroup"
void
writeJson(nlohmann::json json, std::string path, std::string driver)
{
  auto attrs_store = tensorstore::Open<nlohmann::json, 0>(
                       { { "driver", "json" }, { "kvstore", { { "driver", driver }, { "path", path } } } },
                       tsContext,
                       tensorstore::OpenMode::create | tensorstore::OpenMode::delete_existing,
                       tensorstore::ReadWriteMode::read_write)
                       .result()
                       .value();
  auto writeFuture = tensorstore::Write(tensorstore::MakeScalarArray(json), attrs_store); // preferably pretty-print

  auto result = writeFuture.result();
  if (!result.ok())
  {
    itkGenericExceptionMacro(<< "There was an error writing metadata to file '" << path
                             << ". Error details: " << result.status());
  }
}

// JSON file path, e.g. "C:/Dev/ITKIOOMEZarrNGFF/v0.4/cyx.ome.zarr/.zattrs"
bool
jsonRead(std::string path, nlohmann::json & result, std::string driver)
{
  // Reading JSON via TensorStore allows it to be in the cloud
  auto attrs_store = tensorstore::Open<nlohmann::json, 0>(
                       { { "driver", "json" }, { "kvstore", { { "driver", driver }, { "path", path } } } })
                       .result()
                       .value();

  auto attrs_array_result = tensorstore::Read(attrs_store).result();

  nlohmann::json attrs;
  if (attrs_array_result.ok())
  {
    result = attrs_array_result.value()();
    return true;
  }
  else if (absl::IsNotFound(attrs_array_result.status()))
  {
    result = nlohmann::json::object_t();
    return false;
  }
  else // another error
  {
    result = nlohmann::json::object_t();
    return false;
  }
}

bool
OMEZarrNGFFImageIO::CanReadFile(const char * filename)
{
  try
  {
    std::string    driver = getKVstoreDriver(filename);
    nlohmann::json json;
    if (!jsonRead(std::string(filename) + "/.zgroup", json, driver))
    {
      return false;
    }
    if (json.at("zarr_format").get<int>() != 2)
    {
      return false; // unsupported zarr format
    }
    if (!jsonRead(std::string(filename) + "/.zattrs", json, driver))
    {
      return false;
    }
    if (!json.at("multiscales").is_array())
    {
      return false; // multiscales attribute array must be present
    }
    return true;
  }
  catch (...)
  {
    return false;
  }
  // return this->HasSupportedWriteExtension(filename, true);
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


thread_local tensorstore::TensorStore<> store; // initialized by ReadImageInformation/ReadArrayMetadata

void
OMEZarrNGFFImageIO::ReadArrayMetadata(std::string path, std::string driver)
{
  auto openFuture =
    tensorstore::Open({ { "driver", "zarr" }, { "kvstore", { { "driver", driver }, { "path", path } } } },
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

  if (this->GetNumberOfDimensions() == 0) // reading version 0.2 or 0.1
  {
    this->InitializeIdentityMetadata(dims.size());
  }
  else
  {
    assert(this->GetNumberOfDimensions() == dims.size());
  }

  for (unsigned d = 0; d < dims.size(); ++d)
  {
    this->SetDimensions(d, dims[d]);
  }
}

void
addCoordinateTransformations(OMEZarrNGFFImageIO * io, nlohmann::json ct)
{
  assert(ct.is_array());
  assert(ct.size() >= 1);


  assert(ct[0].at("type") == "scale"); // first transformation must be scale
  nlohmann::json s = ct[0].at("scale");
  assert(s.is_array());
  unsigned dim = s.size();
  assert(dim == io->GetNumberOfDimensions());

  for (unsigned d = 0; d < dim; ++d)
  {
    double dS = s[dim - d - 1].get<double>(); // reverse indices KJI into IJK
    io->SetSpacing(d, dS * io->GetSpacing(d));
    io->SetOrigin(d, dS * io->GetOrigin(d)); // TODO: should we update origin like this?
  }

  if (ct.size() > 1) // there is also a translation
  {
    assert(ct[1].at("type") == "translation"); // first transformation must be scale
    nlohmann::json tr = ct[1].at("translation");
    assert(tr.is_array());
    dim = tr.size();
    assert(dim == io->GetNumberOfDimensions());

    for (unsigned d = 0; d < dim; ++d)
    {
      double dOrigin = tr[dim - d - 1].get<double>(); // reverse indices KJI into IJK
      io->SetOrigin(d, dOrigin + io->GetOrigin(d));
    }
  }

  if (ct.size() > 2)
  {
    itkGenericOutputMacro(<< "A sequence of more than 2 transformations is specified in '" << io->GetFileName()
                          << "'. This is currently not supported. Extra transformations are ignored.");
  }
}

thread_local std::string path; // initialized by ReadImageInformation

void
OMEZarrNGFFImageIO::ReadImageInformation()
{
  nlohmann::json json;
  std::string    driver = getKVstoreDriver(this->GetFileName());

  bool status = jsonRead(std::string(this->GetFileName()) + "/.zgroup", json, driver);
  assert(json.at("zarr_format").get<int>() == 2); // only v2 for now
  status = jsonRead(std::string(this->GetFileName()) + "/.zattrs", json, driver);
  json = json.at("multiscales")[0]; // multiscales must be present in OME-NGFF
  auto version = json.at("version").get<std::string>();
  if (version == "0.4" || version == "0.3" || version == "0.2" || version == "0.1")
  {
    // these are explicitly supported versions
  }
  else
  {
    std::string message = "OME-NGFF version " + version + " is not explicitly supported." +
                          "\nImportant features might be ignored." + "\nSupported versions are 0.1 through 0.4.";
    OutputWindowDisplayWarningText(message.c_str());
  }

  if (json.contains("axes")) // optional before 0.3
  {
    this->InitializeIdentityMetadata(json.at("axes").size());
  }
  else
  {
    this->SetNumberOfDimensions(0);
  }

  if (json.contains("coordinateTransformations")) // optional
  {
    addCoordinateTransformations(this, json.at("coordinateTransformations")); // dataset-level scaling
  }
  json = json.at("datasets");
  if (this->GetDatasetIndex() > json.size())
  {
    itkExceptionMacro(<< "Requested DatasetIndex of " << this->GetDatasetIndex()
                      << " is greater than number of datasets (" << json.size() << ") which exist in OME-NGFF store '"
                      << this->GetFileName() << "'");
  }

  json = json[this->GetDatasetIndex()];
  if (json.contains("coordinateTransformations")) // optional for versions prior to 0.4
  {
    addCoordinateTransformations(this, json.at("coordinateTransformations")); // per-resolution scaling
  }
  else
  {
    if (version == "0.4")
    {
      itkExceptionMacro(<< "OME-NGFF v0.4 requires `coordinateTransformations` for each resolution level.");
    }
  }

  path = json.at("path").get<std::string>();

  // TODO: parse stuff from "metadata" object into metadata dictionary

  ReadArrayMetadata(std::string(this->GetFileName()) + "/" + path, driver);
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
    else
    {
      itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType()));
    }
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
  std::string driver = getKVstoreDriver(this->GetFileName());

  nlohmann::json group;
  group["zarr_format"] = 2;
  writeJson(group, std::string(this->GetFileName()) + "/.zgroup", driver);

  unsigned dim = this->GetNumberOfDimensions();

  std::vector<double> origin(dim);
  std::vector<double> spacing(dim);

  std::vector<nlohmann::json> axes(dim);
  for (unsigned d = 0; d < dim; ++d)
  {
    // reverse indices IJK into KJI
    nlohmann::json dAxis = { { "name", this->dimensionNames[dim - d - 1] },
                             { "type", this->dimensionTypes[dim - d - 1] },
                             { "unit", this->dimensionUnits[dim - d - 1] } };
    axes[d] = dAxis;
    origin[d] = this->GetOrigin(dim - d - 1);
    spacing[d] = this->GetSpacing(dim - d - 1);
  }

  path = "s" + std::to_string(this->GetDatasetIndex());

  nlohmann::json datasets = { { "coordinateTransformations",
                                { { { "scale", spacing }, { "type", "scale" } },
                                  { { "translation", origin }, { "type", "translation" } } } },
                              { "path", path } };

  nlohmann::json multiscales = {
    { { "axes", axes }, { "datasets", { datasets } }, { "version", "0.4" } },
  };

  // TODO: add stuff from metadata dictionary into "metadata" object

  nlohmann::json zattrs;
  zattrs["multiscales"] = multiscales;
  writeJson(zattrs, std::string(this->GetFileName()) + "/.zattrs", driver);
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
        { "kvstore", { { "driver", driver }, { "path", this->m_FileName + "/" + path } } },           \
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
  std::filesystem::path file(this->GetFileName());
  if (std::filesystem::is_regular_file(file))
  {
    // work around current limitation of TensorStore's ZIP support
    // by deleting the existing zip file
    std::filesystem::remove(file);
  }

  this->WriteImageInformation();

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

  std::string driver = getKVstoreDriver(this->GetFileName());

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
  else
  {
    itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType()));
  }
}

} // end namespace itk
