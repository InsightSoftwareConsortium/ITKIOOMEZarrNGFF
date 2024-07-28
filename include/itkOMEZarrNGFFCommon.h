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

} // end namespace itk

#endif // itkOMEZarrNGFFCommon_h
