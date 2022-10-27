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
#include "itkImageIOBase.h"

namespace itk
{
/** \class OMEZarrNGFFImageIO
 *
 * \brief Read and write OMEZarrNGFF images.
 *
 * Open Microscopy Environment Zarr Next Generation File Format
 * specification can be found at https://github.com/ome/ngff
 *
 * \ingroup IOFilters
 * \ingroup IOOMEZarrNGFF
 */
class IOOMEZarrNGFF_EXPORT OMEZarrNGFFImageIO : public ImageIOBase
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(OMEZarrNGFFImageIO);

  /** Standard class typedefs. */
  using Self = OMEZarrNGFFImageIO;
  using Superclass = ImageIOBase;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(OMEZarrNGFFImageIO, ImageIOBase);

  static constexpr unsigned MaximumDimension = 5; // OME-NGFF specifies up to 5D data

  /** The different types of ImageIO's can support data of varying
   * dimensionality. For example, some file formats are strictly 2D
   * while others can support 2D, 3D, or even n-D. This method returns
   * true/false as to whether the ImageIO can support the dimension
   * indicated. */
  bool
  SupportsDimension(unsigned long dimension) override
  {
    if (dimension <= MaximumDimension)
    {
      return true;
    }
    return false;
  }

  /*-------- This part of the interfaces deals with reading data. ----- */

  /** Determine the file type. Returns true if this ImageIO can read the
   * file specified. */
  bool
  CanReadFile(const char *) override;

  /** Set the spacing and dimension information for the set filename. */
  void
  ReadImageInformation() override;

  /** Reads the data from disk into the memory buffer provided. */
  void
  Read(void * buffer) override;

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine the file type. Returns true if this ImageIO can write the
   * file specified. */
  bool
  CanWriteFile(const char *) override;

  /** Set the spacing and dimension information for the set filename. */
  void
  WriteImageInformation() override;

  /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegions has been set properly. */
  void
  Write(const void * buffer) override;


  bool
  CanStreamRead() override
  {
    return true;
  }

  bool
  CanStreamWrite() override
  {
    return true;
  }

protected:
  OMEZarrNGFFImageIO();
  ~OMEZarrNGFFImageIO() override = default;

  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  ImageIORegion
  GetLargestRegion()
  {
    const unsigned int nDims = this->GetNumberOfDimensions();
    ImageIORegion      largestRegion(nDims);
    for (unsigned int i = 0; i < nDims; ++i)
    {
      largestRegion.SetIndex(i, 0);
      largestRegion.SetSize(i, this->GetDimensions(i));
    }
    return largestRegion;
  }

  const std::vector<std::string> dimensionNames = { "x", "y", "z", "c", "t" };

private:
  // We don't need any private members here
};
} // end namespace itk

#endif // itkOMEZarrNGFFImageIO_h
