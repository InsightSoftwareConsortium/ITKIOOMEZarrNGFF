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

#ifndef itkOMEZarrNGFFCommon_h
#define itkOMEZarrNGFFCommon_h
#include "IOOMEZarrNGFFExport.h"

#include <string>

#include "itkIOCommon.h"

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
/** \class OMEZarrNGFFAxis
 *
 * \brief Represent an OME-Zarr NGFF axis
 *
 * Open Microscopy Environment Zarr Next Generation File Format
 * specification can be found at https://github.com/ome/ngff
 *
 * \ingroup IOOMEZarrNGFF
 */
struct IOOMEZarrNGFF_EXPORT OMEZarrAxis
{
  std::string name;
  std::string type;
  std::string unit;
};

// Update an existing "read" specification for an "http" driver to retrieve remote files.
// Note that an "http" driver specification may operate on an HTTP or HTTPS connection.
void
makeKVStoreHTTPDriverSpec(nlohmann::json & spec, const std::string & fullPath);

tensorstore::DataType
itkToTensorstoreComponentType(const IOComponentEnum itkComponentType);

// Returns TensorStore KvStore driver name appropriate for this path.
// Options are file, zip. TODO: http, gcs (GoogleCouldStorage), etc.
std::string
getKVstoreDriver(std::string path);

// JSON file path, e.g. "C:/Dev/ITKIOOMEZarrNGFF/v0.4/cyx.ome.zarr/.zgroup"
void
writeJson(nlohmann::json json, std::string path, std::string driver);

// JSON file path, e.g. "C:/Dev/ITKIOOMEZarrNGFF/v0.4/cyx.ome.zarr/.zattrs"
bool
jsonRead(const std::string path, nlohmann::json & result, std::string driver);

// We call tensorstoreToITKComponentType for each type.
// Hopefully compiler will optimize it away via constant propagation and inlining.
#define READ_ELEMENT_IF(typeName)                                                                     \
  else if (tensorstoreToITKComponentType(tensorstore::dtype_v<typeName>) == this->GetComponentType()) \
  {                                                                                                   \
    ReadFromStore<typeName>(store, storeIORegion, reinterpret_cast<typeName *>(buffer));              \
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

} // end namespace itk

#endif // itkOMEZarrNGFFCommon_h
