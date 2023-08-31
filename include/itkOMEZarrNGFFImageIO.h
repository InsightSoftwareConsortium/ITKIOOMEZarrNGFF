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
struct IOOMEZarrNGFF_EXPORT OMEZarrNGFFAxis
{
  std::string name;
  std::string type;
  std::string unit;
};

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
  static constexpr int      INVALID_INDEX = -1;   // for specifying enumerated axis slice indices
  using AxesCollectionType = std::vector<OMEZarrNGFFAxis>;

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
  MakeMemoryFileName(const itk::OMEZarrNGFFImageIO::BufferInfo & bufferInfo)
  {
    size_t bufferInfoAddress = reinterpret_cast<size_t>(&bufferInfo);
    return std::to_string(bufferInfoAddress) + ".memory";
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

  /** Method for supporting streaming.  Given a requested region, determine what
   * could be the region that we can read from the file. This is called the
   * streamable region, which will be smaller than the LargestPossibleRegion and
   * greater or equal to the RequestedRegion.
   *
   * Under current behavior this simply propagates the requested region.
   * Could be extended in the future to support chunk-based streaming.
   */
  ImageIORegion
  GenerateStreamableReadRegionFromRequestedRegion(const ImageIORegion & requestedRegion) const override;

  /** Which resolution level is desired? */
  itkGetConstMacro(DatasetIndex, int);
  itkSetMacro(DatasetIndex, int);

  /** If there is a time axis, at what index should it be sliced? */
  itkGetConstMacro(TimeIndex, int);
  itkSetMacro(TimeIndex, int);

  /** If there are multiple channels, which one should be read? */
  itkGetConstMacro(ChannelIndex, int);
  itkSetMacro(ChannelIndex, int);

  /** Get the available axes in the OME-Zarr store in ITK (Fortran-style) order.
   *  This is reversed from the default C-style order of
   *  axes as used in the Zarr / NumPy / Tensorstore interface.
   */
  itkGetConstMacro(StoreAxes, const AxesCollectionType &);

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

  /** Read a single array and set relevant metadata. */
  void
  ReadArrayMetadata(std::string path, std::string driver);

  /** Process requested store region for given configuration */
  ImageIORegion
  ConfigureTensorstoreIORegion(const ImageIORegion & ioRegion) const;

  /** Helper method to get axes in tensorstore C-style order*/
  AxesCollectionType
  GetAxesInStoreOrder() const
  {
    return AxesCollectionType(m_StoreAxes.rbegin(), m_StoreAxes.rend());
  }

  /** Sets the requested dimension, and initializes spatial metadata to identity. */
  void
  InitializeIdentityMetadata(unsigned nDims)
  {
    this->SetNumberOfDimensions(nDims);

    // initialize identity transform
    for (unsigned d = 0; d < this->GetNumberOfDimensions(); ++d)
    {
      this->SetSpacing(d, 1.0);
      this->SetOrigin(d, 0.0);
      this->SetDirection(d, this->GetDefaultDirection(d));
    }
  }

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
  const std::vector<std::string> dimensionTypes = { "space", "space", "space", "channel", "time" };
  const std::vector<std::string> dimensionUnits = { "millimeter", "millimeter", "millimeter", "index", "second" };

private:
  int                m_DatasetIndex = 0; // first, highest resolution scale by default
  int                m_TimeIndex = INVALID_INDEX;
  int                m_ChannelIndex = INVALID_INDEX;
  AxesCollectionType m_StoreAxes;

  const static unsigned m_EmptyZipSize = 22;
  char                  m_EmptyZip[m_EmptyZipSize] = "PK\x05\x06"; // the rest is filled with zeroes
  const BufferInfo      m_EmptyZipBufferInfo{ m_EmptyZip, m_EmptyZipSize };
  const std::string     m_EmptyZipFileName = MakeMemoryFileName(m_EmptyZipBufferInfo);
};
} // end namespace itk

#endif // itkOMEZarrNGFFImageIO_h
