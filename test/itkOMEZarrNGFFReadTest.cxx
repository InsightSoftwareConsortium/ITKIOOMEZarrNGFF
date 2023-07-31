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

/// This test utility may be used to validate that an OME-Zarr image can be
/// read from disk or from an HTTP source.
///
/// No attempt is made to validate input data. A summary of the retrieved image
/// is printed to `std::cout`.
///
/// Does not currently support multichannel sources.
/// https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/issues/32

#include <fstream>
#include <string>

#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkOMEZarrNGFFImageIO.h"
#include "itkOMEZarrNGFFImageIOFactory.h"
#include "itkTestingMacros.h"
#include "itkImageIOBase.h"

namespace
{
template <typename PixelType = unsigned char, size_t ImageDimension = 3>
void
doRead(const std::string & path, const int datasetIndex)
{
  using ImageType = itk::Image<PixelType, ImageDimension>;
  auto imageReader = itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(path);

  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(datasetIndex);
  imageReader->SetImageIO(imageIO);

  imageReader->UpdateOutputInformation();
  auto output = imageReader->GetOutput();
  output->Print(std::cout);

  imageReader->Update();
  output->Print(std::cout);
}
} // namespace

int
itkOMEZarrNGFFReadTest(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " Input <ImageDimension> <DatasetIndex> [NumChannels]"
              << std::endl;
    return EXIT_FAILURE;
  }
  const char * inputFileName = argv[1];
  const size_t imageDimension = (argc > 2 ? std::atoi(argv[2]) : 3);
  const size_t datasetIndex = (argc > 3 ? std::atoi(argv[3]) : 0);
  const size_t numChannels = (argc > 4 ? std::atoi(argv[4]) : 1);

  if (numChannels != 1)
  {
    throw std::runtime_error("Multichannel image reading is not currently supported");
  }
  else if (imageDimension == 2)
  {
    doRead<unsigned char, 2>(inputFileName, datasetIndex);
  }
  else if (imageDimension == 3)
  {
    doRead<unsigned char, 3>(inputFileName, datasetIndex);
  }
  else if (imageDimension == 4)
  {
    doRead<unsigned char, 4>(inputFileName, datasetIndex);
  }
  else
  {
    throw std::invalid_argument("Received an invalid test case");
  }

  return EXIT_SUCCESS;
}
