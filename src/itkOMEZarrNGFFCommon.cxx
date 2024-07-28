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

#include "itkOMEZarrNGFFCommon.h"
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

thread_local tensorstore::Context tsContext = tensorstore::Context::Default();

// Update an existing "read" specification for an "http" driver to retrieve remote files.
// Note that an "http" driver specification may operate on an HTTP or HTTPS connection.
void
makeKVStoreHTTPDriverSpec(nlohmann::json & spec, const std::string & fullPath)
{
  // Decompose path into a base URL and reference subpath according to TensorStore HTTP KVStore driver spec
  // https://google.github.io/tensorstore/kvstore/http/index.html
  spec["kvstore"] = { { "driver", "http" } };

  // Naively decompose the URL into "base" and "resource" components.
  // Generally assumes that the spec will only be used once to access a specific resource.
  // For example, the URL "http://localhost/path/to/resource.json" will be split
  // into components "http://localhost/path/to" and "resource.json".
  //
  // Could be revisited for a better root "base_url" at the top level allowing acces
  // to multiple subpaths. For instance, decomposing the example above into
  // "http://localhost/" and "path/to/resource.json" would allow for a given HTTP spec
  // to be more easily reused with different subpaths.
  //
  spec["kvstore"]["base_url"] = fullPath.substr(0, fullPath.find_last_of("/"));
  spec["kvstore"]["path"] = fullPath.substr(fullPath.find_last_of("/") + 1);
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
  if (path.substr(0, 4) == "http")
  { // http or https
    return "http";
  }
  if (path.substr(path.size() - 4) == ".zip" || path.substr(path.size() - 7) == ".memory")
  {
    return "zip_memory";
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
jsonRead(const std::string path, nlohmann::json & result, std::string driver)
{
  // Reading JSON via TensorStore allows it to be in the cloud
  nlohmann::json readSpec = { { "driver", "json" }, { "kvstore", { { "driver", driver }, { "path", path } } } };
  if (driver == "http")
  {
    makeKVStoreHTTPDriverSpec(readSpec, path);
  }

  auto attrs_store = tensorstore::Open<nlohmann::json, 0>(readSpec, tsContext).result().value();

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

} // end namespace itk
