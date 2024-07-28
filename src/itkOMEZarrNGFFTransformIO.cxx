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

#define ITK_TEMPLATE_EXPLICIT_WasmTransformIO
#include "itkOMEZarrNGFFTransformIO.h"
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

template <typename TParametersValueType>
OMEZarrNGFFTransformIOTemplate<TParametersValueType>::~OMEZarrNGFFTransformIOTemplate() = default;

template <typename TParametersValueType>
OMEZarrNGFFTransformIOTemplate::OMEZarrNGFFTransformIO()
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


template <typename TParametersValueType>
void
OMEZarrNGFFTransformIOTemplate<TParametersValueType>
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template <typename TParametersValueType>
void
bool
OMEZarrNGFFTransformIO::CanReadFile(const char * filename)
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


thread_local tensorstore::TensorStore<> store; // initialized by ReadTransformInformation/ReadArrayMetadata

thread_local std::string path; // initialized by ReadTransformInformation

template <typename TParametersValueType>
void
OMEZarrNGFFTransformIOTemplate<TParametersValueType>::Read()
{
}


template <typename TParametersValueType>
bool
OMEZarrNGFFTransformIOTemplate<TParametersValueType>::CanWriteFile(const char * name)
{
}

template <typename TParametersValueType>
void
OMEZarrNGFFTransformIOTemplate<TParametersValueType>::Write()
{
}


} // end namespace itk
