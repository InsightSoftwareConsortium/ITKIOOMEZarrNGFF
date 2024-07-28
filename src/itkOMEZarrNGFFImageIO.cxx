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
#include "itkMacro.h"

#include "tensorstore/container_kind.h"
#include "tensorstore/context.h"
#include "tensorstore/index_space/dim_expression.h"
#include "tensorstore/open.h"
#include "tensorstore/index_space/index_domain.h"
#include "tensorstore/index_space/index_domain_builder.h"
#include "tensorstore/index_space/dim_expression.h"

#include <nlohmann/json.hpp>

namespace itk
{
namespace
{
template <typename TPixel>
void
ReadFromStore(const tensorstore::TensorStore<> & store, const ImageIORegion & storeIORegion, TPixel * buffer)
{
  if (store.domain().num_elements() == storeIORegion.GetNumberOfPixels())
  {
    // Read the entire available voxel region.
    // Allow tensorstore to perform any axis permutations or other index operations
    // to map from store axes to ITK image axes.
    auto arr = tensorstore::Array(buffer, store.domain().shape(), tensorstore::c_order);
    tensorstore::Read(store, tensorstore::UnownedToShared(arr)).value();
  }
  else
  {
    // Read a requested voxel subregion.
    // We cannot infer axis permutations by matching requested axis sizes.
    // Therefore we assume that tensorstore axes are in "C-style" order
    // with the last index as the fastest moving axis, aka "z,y,x" order,
    // and must inverted to match ITK's "Fortran-style" order of axis indices
    // with the first index as the fastest moving axis, aka "x,y,z" order.
    // "C-style" is generally the default layout for new tensorstore arrays.
    // Refer to https://google.github.io/tensorstore/driver/zarr/index.html#json-driver/zarr.metadata.order
    //
    // In the future this may be extended to permute axes based on
    // OME-Zarr NGFF axis labels.
    const auto                      dimension = store.rank();
    std::vector<tensorstore::Index> indices(dimension);
    std::vector<tensorstore::Index> sizes(dimension);
    for (size_t dim = 0; dim < dimension; ++dim)
    {
      // Input IO region is assumed to already be reversed from ITK requested region
      // to match assumed C-style Zarr storage
      indices[dim] = storeIORegion.GetIndex(dim);
      sizes[dim] = storeIORegion.GetSize(dim);
    }
    auto indexDomain = tensorstore::IndexDomainBuilder(dimension).origin(indices).shape(sizes).Finalize().value();

    auto arr = tensorstore::Array(buffer, indexDomain.shape(), tensorstore::c_order);
    auto indexedStore = store | tensorstore::AllDims().SizedInterval(indices, sizes);
    tensorstore::Read(indexedStore, tensorstore::UnownedToShared(arr)).value();
  }
}


} // namespace

OMEZarrNGFFImageIO::OMEZarrNGFFImageIO()
{
  this->AddSupportedWriteExtension(".zarr");
  this->AddSupportedWriteExtension(".zr2");
  this->AddSupportedWriteExtension(".zr3");
  this->AddSupportedWriteExtension(".zip");
  this->AddSupportedWriteExtension(".memory");

  this->AddSupportedReadExtension(".zarr");
  this->AddSupportedReadExtension(".zr2");
  this->AddSupportedReadExtension(".zr3");
  this->AddSupportedReadExtension(".zip");
  this->AddSupportedWriteExtension(".memory");

  this->Self::SetCompressor("");
  this->Self::SetMaximumCompressionLevel(9);
  this->Self::SetCompressionLevel(2);
}


void
OMEZarrNGFFImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
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
  nlohmann::json readSpec = { { "driver", "zarr" }, { "kvstore", { { "driver", driver }, { "path", path } } } };
  if (driver == "http")
  {
    makeKVStoreHTTPDriverSpec(readSpec, path);
  }

  auto openFuture = tensorstore::Open(readSpec,
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
    itkAssertOrThrowMacro(this->GetNumberOfDimensions() == dims.size(), "Found dimension mismatch in metadata");
  }

  for (unsigned d = 0; d < dims.size(); ++d)
  {
    this->SetDimensions(d, dims[d]);
  }
}

ImageIORegion
OMEZarrNGFFImageIO::ConfigureTensorstoreIORegion(const ImageIORegion & ioRegion) const
{
  // Set up IO region to match known store dimensions
  itkAssertOrThrowMacro(m_StoreAxes.size() == store.rank(), "Detected mismatch in axis count and store rank");
  ImageIORegion storeRegion(store.rank());
  itkAssertOrThrowMacro(storeRegion.GetImageDimension(), store.rank());
  auto storeAxes = this->GetAxesInStoreOrder();

  for (size_t storeIndex = 0; storeIndex < store.rank(); ++storeIndex)
  {
    auto axisName = storeAxes.at(storeIndex).name;

    // Optionally slice time or channel indices
    if (axisName == "t")
    {
      storeRegion.SetSize(storeIndex, 1);
      if (m_TimeIndex == INVALID_INDEX)
      {
        itkWarningMacro(<< "The OME-Zarr store contains a time \"t\" axis but no time point has been specified. "
                           "Reading along a time axis is not currently supported. Data will be read from the first "
                           "available time point by default.");
        storeRegion.SetIndex(storeIndex, 0);
      }
      else
      {
        storeRegion.SetIndex(storeIndex, m_TimeIndex);
      }
    }
    else if (axisName == "c")
    {
      storeRegion.SetSize(storeIndex, 1);
      if (m_ChannelIndex == INVALID_INDEX)
      {
        itkWarningMacro(<< "The OME-Zarr store contains a channel \"c\" axis but no channel index has been specified. "
                           "Reading along a channel axis is not currently supported. Data will be read from the first "
                           "available channel by default.");
        storeRegion.SetIndex(storeIndex, 0);
      }
      else
      {
        storeRegion.SetIndex(storeIndex, m_ChannelIndex);
      }
    }
    // Set requested region on X/Y/Z axes
    else if (axisName == "x")
    {
      itkAssertOrThrowMacro(ioRegion.GetImageDimension() >= 1, "Failed to read from \"x\" axis into ITK axis \"0\"");
      storeRegion.SetSize(storeIndex, ioRegion.GetSize(0));
      storeRegion.SetIndex(storeIndex, ioRegion.GetIndex(0));
    }
    else if (axisName == "y")
    {
      itkAssertOrThrowMacro(ioRegion.GetImageDimension() >= 2, "Failed to read from \"y\" axis into ITK axis \"1\"");
      storeRegion.SetSize(storeIndex, ioRegion.GetSize(1));
      storeRegion.SetIndex(storeIndex, ioRegion.GetIndex(1));
    }
    else if (axisName == "z")
    {
      itkAssertOrThrowMacro(ioRegion.GetImageDimension() >= 3, "Failed to read from \"z\" axis into ITK axis \"2\"");
      storeRegion.SetSize(storeIndex, ioRegion.GetSize(2));
      storeRegion.SetIndex(storeIndex, ioRegion.GetIndex(2));
    }
  }

  return storeRegion;
}

void
addCoordinateTransformations(OMEZarrNGFFImageIO * io, nlohmann::json ct)
{
  itkAssertOrThrowMacro(ct.is_array(), "Failed to parse coordinate transforms");
  itkAssertOrThrowMacro(ct.size() >= 1, "Expected at least one coordinate transform");
  itkAssertOrThrowMacro(ct[0].at("type") == "scale",
                        ("Expected first transform to be \"scale\" but found " +
                         std::string(ct[0].at("type")))); // first transformation must be scale

  nlohmann::json s = ct[0].at("scale");
  itkAssertOrThrowMacro(s.is_array(), "Failed to parse scale transform");
  unsigned dim = s.size();
  itkAssertOrThrowMacro(dim == io->GetNumberOfDimensions(), "Found dimension mismatch in scale transform");

  for (unsigned d = 0; d < dim; ++d)
  {
    double dS = s[dim - d - 1].get<double>(); // reverse indices KJI into IJK
    io->SetSpacing(d, dS * io->GetSpacing(d));
    io->SetOrigin(d, dS * io->GetOrigin(d)); // TODO: should we update origin like this?
  }

  if (ct.size() > 1) // there is also a translation
  {
    itkAssertOrThrowMacro(ct[1].at("type") == "translation",
                          ("Expected second transform to be \"translation\" but found " +
                           std::string(ct[1].at("type")))); // first transformation must be scale
    nlohmann::json tr = ct[1].at("translation");
    itkAssertOrThrowMacro(tr.is_array(), "Failed to parse translation transform");
    dim = tr.size();
    itkAssertOrThrowMacro(dim == io->GetNumberOfDimensions(), "Found dimension mismatch in translation transform");

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

  const std::string zgroupFilePath(std::string(this->GetFileName()) + "/.zgroup");
  bool              status = jsonRead(zgroupFilePath, json, driver);
  itkAssertOrThrowMacro(status, ("Failed to read from " + zgroupFilePath));
  itkAssertOrThrowMacro(json.at("zarr_format").get<int>() == 2, "Only v2 zarr format is supported"); // only v2 for now

  const std::string zattrsFilePath(std::string(this->GetFileName()) + "/.zattrs");
  status = jsonRead(zattrsFilePath, json, driver);
  itkAssertOrThrowMacro(status, ("Failed to read from " + zattrsFilePath));
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

    m_StoreAxes.resize(json.at("axes").size());
    auto targetIt = m_StoreAxes.rbegin();
    for (const auto & axis : json.at("axes"))
    {
      *targetIt = (OMEZarrAxis{ axis.at("name"), axis.at("type"), (axis.contains("unit") ? axis.at("unit") : "") });
      ++targetIt;
    }
    itkAssertOrThrowMacro(targetIt == m_StoreAxes.rend(),
                          "Internal error: failed to fully parse axes from OME-Zarr metadata");
  }
  else
  {
    if (version == "0.4")
    {
      itkExceptionMacro(<< "\"axes\" field is missing from OME-Zarr image metadata at " << zattrsFilePath);
    }
    this->SetNumberOfDimensions(0);
  }

  if (json.contains("coordinateTransformations")) // optional
  {
    addCoordinateTransformations(this, json.at("coordinateTransformations")); // dataset-level scaling
  }
  json = json.at("datasets");
  if (this->GetDatasetIndex() >= json.size())
  {
    itkExceptionMacro(<< "Requested DatasetIndex of " << this->GetDatasetIndex()
                      << " is out of range for the number of datasets (" << json.size()
                      << ") which exist in OME-NGFF store '" << this->GetFileName() << "'");
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

void
OMEZarrNGFFImageIO::Read(void * buffer)
{
  // Use a proxy measure (voxel count) to determine whether we are reading
  // the entire image or an image subregion.
  // This comparison needs to be done carefully, we can compare 3D and 6D regions
  if (this->GetLargestRegion().GetNumberOfPixels() == m_IORegion.GetNumberOfPixels())
  {
    itkAssertOrThrowMacro(store.domain().num_elements() == m_IORegion.GetNumberOfPixels(),
                          "Detected mismatch between store size and size of largest possible region");
  }
  else
  {
    // Get a requested image subregion
    itkAssertOrThrowMacro(this->GetNumberOfComponents() == 1,
                          "Reading an image subregion is currently supported only for single channel images");
  }
  auto storeIORegion = this->ConfigureTensorstoreIORegion(m_IORegion);

  if (this->GetDebug())
  {
    std::cout << "Preparing to read " << storeIORegion.GetNumberOfPixels() << " elements from tensorstore region "
              << storeIORegion;
  }

  if (false)
  {
  }
  READ_ELEMENT_IF(float)
  READ_ELEMENT_IF(int8_t)
  READ_ELEMENT_IF(uint8_t)
  READ_ELEMENT_IF(int16_t)
  READ_ELEMENT_IF(uint16_t)
  READ_ELEMENT_IF(int32_t)
  READ_ELEMENT_IF(uint32_t)
  READ_ELEMENT_IF(int64_t)
  READ_ELEMENT_IF(uint64_t)
  READ_ELEMENT_IF(float)
  READ_ELEMENT_IF(double)
  else
  {
    itkExceptionMacro("Unsupported component type: " << GetComponentTypeAsString(this->GetComponentType()));
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


void
OMEZarrNGFFImageIO::Write(const void * buffer)
{
  if (m_FileName.substr(m_FileName.size() - 4) == ".zip" || m_FileName.substr(m_FileName.size() - 7) == ".memory")
  {
    tsContext = tensorstore::Context::Default(); // start with clean zip handles
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
  {
  } // so element statements can all be "else if"
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

  if (m_FileName.substr(m_FileName.size() - 4) == ".zip" || m_FileName.substr(m_FileName.size() - 7) == ".memory")
  {
    // Attempt to read a non-existent file from the in-memory zip to close the current one
    nlohmann::json temp;
    bool           wasRead = jsonRead(m_EmptyZipFileName + "/non-existent.json", temp, "zip_memory");
    assert(wasRead == false);
  }
}


ImageIORegion
OMEZarrNGFFImageIO::GenerateStreamableReadRegionFromRequestedRegion(const ImageIORegion & requestedRegion) const
{
  // Propagate the requested region
  return requestedRegion;
}

} // end namespace itk
