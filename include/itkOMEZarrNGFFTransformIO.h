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

#ifndef itkOMEZarrNGFFImageIO_h
#define itkOMEZarrNGFFImageIO_h
#include "IOOMEZarrNGFFExport.h"


#include <fstream>
#include <string>
#include <vector>

#include "itkImageIOBase.h"

#include "itkOMEZarrNGFFCommon.h"

namespace itk
{

/** \class OMEZarrNGFFTransformIO
 *
 * \brief Read and write OME-Zarr coordinate transformations.
 *
 * Open Microscopy Environment Zarr Next Generation File Format
 * specification can be found at https://github.com/ome/ngff
 *
 * \ingroup IOFilters
 * \ingroup IOOMEZarrNGFF
 */
template <typename TParametersValueType>
class ITK_TEMPLATE_EXPORT OMEZarrNGFFTransformIOTemplate : public TransformIOBaseTemplate<TParametersValueType>
{
public:
  /** Standard class typedefs. */
  using Self = OMEZarrNGFFTransformIOTemplate;
  using Superclass = TransformIOBaseTemplate<TParametersValueType>;
  using Pointer = SmartPointer<Self>;
  using typename Superclass::TransformListType;
  using typename Superclass::TransformPointer;
  using typename Superclass::TransformType;
  using ParametersType = typename TransformType::ParametersType;
  using ParametersValueType = typename TransformType::ParametersValueType;
  using FixedParametersType = typename TransformType::FixedParametersType;
  using FixedParametersValueType = typename TransformType::FixedParametersValueType;

  using ConstTransformListType = typename TransformIOBaseTemplate<ParametersValueType>::ConstTransformListType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(OMEZarrNGFFTransformIOTemplate, TransformIOBaseTemplate);

  static constexpr unsigned MaximumDimension = 5; // OME-NGFF specifies up to 5D data
  static constexpr int      INVALID_INDEX = -1;   // for specifying enumerated axis slice indices
  using AxesCollectionType = std::vector<OMEZarrAxis>;

  /** Special in-memory zip interface. An address needs to be provided in
   * the "file name", using pattern address.memory, where address is a
   * decimal representation of BufferInfo's address.
   * Sample filename: "12341234.memory". */
  using BufferInfo = struct
  {
    char * pointer;
    size_t size;
  };

  /** Construct a "magic" file name from the provided bufferInfo. */
  static std::string
  MakeMemoryFileName(const BufferInfo & bufferInfo)
  {
    size_t bufferInfoAddress = reinterpret_cast<size_t>(&bufferInfo);
    return std::to_string(bufferInfoAddress) + ".memory";
  }

  /*-------- This part of the interfaces deals with reading data. ----- */

  /** Reads the data from disk into the memory buffer provided. */
  void
  Read() override;

  bool
  CanReadFile(const char *) override;

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine the file type. Returns true if this TransformIO can write the
   * file specified. */
  bool
  CanWriteFile(const char *) override;

  void
  Write() override;

protected:
  OMEZarrNGFFTransformIOTemplate();
  ~OMEZarrNGFFTransformIOTemplate() override = default;

  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  /** Read a single array and set relevant metadata. */
  void
  ReadArrayMetadata(std::string path, std::string driver);

private:
  // An empty zip file consists of 22 bytes of "end of central directory" record. More:
  // https://github.com/google/tensorstore/blob/45565464b9f9e2567144d780c3bef365ee3c125a/tensorstore/internal/compression/zip_details.h#L64-L76
  constexpr static unsigned m_EmptyZipSize = 22;
  char                      m_EmptyZip[m_EmptyZipSize] = "PK\x05\x06"; // the rest is filled with zeroes
  const BufferInfo          m_EmptyZipBufferInfo{ m_EmptyZip, m_EmptyZipSize };
  const std::string         m_EmptyZipFileName = MakeMemoryFileName(m_EmptyZipBufferInfo);

  ITK_DISALLOW_COPY_AND_MOVE(OMEZarrNGFFTransformIOTemplate);
};
} // end namespace itk

#endif // itkOMEZarrNGFFImageIO_h
