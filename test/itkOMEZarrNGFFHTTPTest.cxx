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

// Read an OME-Zarr image from a remote store.
// Example data is available at https://github.com/ome/ome-ngff-prototypes

#include <fstream>
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkOMEZarrNGFFImageIO.h"
#include "itkOMEZarrNGFFImageIOFactory.h"
#include "itkTestingMacros.h"
#include "itkImageIOBase.h"

namespace
{
static constexpr bool USE_MHA_COMPRESSION = true;

std::string
makeOutputName(const std::string & outputPrefix, size_t datasetIndex)
{
  return outputPrefix + "_" + std::to_string(datasetIndex) + ".mha";
}

template <typename TImagePointer>
void
writeOutputImage(TImagePointer image, const std::string & outputPrefix, size_t datasetIndex)
{
  std::string outputFilename = makeOutputName(outputPrefix, datasetIndex);
  itk::WriteImage(image, outputFilename, USE_MHA_COMPRESSION);
}

bool
test2DImage(const std::string & outputPrefix)
{
  using ImageType = itk::Image<unsigned char, 2>;
  const std::string resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/yx.ome.zarr";

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  size_t resolution = 0;
  auto   image = itk::ReadImage<ImageType>(resourceURL);
  image->Print(std::cout);

  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), image->GetBufferedRegion());
  writeOutputImage(image, outputPrefix, resolution);

  resolution = 1;
  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(resolution);
  auto reader1 = itk::ImageFileReader<ImageType>::New();
  reader1->SetFileName(resourceURL);
  reader1->SetImageIO(imageIO);
  reader1->Update();
  image = reader1->GetOutput();
  image->Print(std::cout);
  writeOutputImage(image, outputPrefix, resolution);

  // Resolution 2
  imageIO->SetDatasetIndex(resolution);
  auto reader2 = itk::ImageFileReader<ImageType>::New();
  reader2->SetFileName(resourceURL);
  reader2->SetImageIO(imageIO);
  reader2->Update();
  image = reader2->GetOutput();
  image->Print(std::cout);
  writeOutputImage(image, outputPrefix, resolution);

  return EXIT_SUCCESS;
}

bool
test3DImage(const std::string & outputPrefix)
{
  using ImageType = itk::Image<unsigned char, 3>;
  const std::string resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/zyx.ome.zarr";

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  size_t resolution = 0;
  auto   image = itk::ReadImage<ImageType>(resourceURL);
  image->Print(std::cout);
  writeOutputImage(image, outputPrefix, resolution);

  resolution = 1;
  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(resolution);
  auto reader1 = itk::ImageFileReader<ImageType>::New();
  reader1->SetFileName(resourceURL);
  reader1->SetImageIO(imageIO);
  reader1->Update();
  image = reader1->GetOutput();
  image->Print(std::cout);
  writeOutputImage(image, outputPrefix, resolution);

  resolution = 2;
  imageIO->SetDatasetIndex(resolution);
  auto reader2 = itk::ImageFileReader<ImageType>::New();
  reader2->SetFileName(resourceURL);
  reader2->SetImageIO(imageIO);
  reader2->Update();
  image = reader2->GetOutput();
  image->Print(std::cout);
  writeOutputImage(image, outputPrefix, resolution);

  return EXIT_SUCCESS;
}

bool
testTimeSlice(const std::string & outputPrefix)
{
  // Read a subregion of an arbitrary time point from a 3D image buffer into a 2D image
  using ImageType = itk::Image<unsigned char, 2>;
  const std::string       resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/tyx.ome.zarr";
  static constexpr size_t RESOLUTION = 0;
  static constexpr size_t TIME_INDEX = 2;

  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(RESOLUTION);
  imageIO->SetTimeIndex(TIME_INDEX);

  auto requestedRegion = itk::ImageRegion<2>();
  requestedRegion.SetSize(itk::MakeSize(50, 50));
  requestedRegion.SetIndex(itk::MakeIndex(100, 100));

  // Set up reader
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(resourceURL);
  reader->SetImageIO(imageIO);
  reader->GetOutput()->SetRequestedRegion(requestedRegion);

  // Read
  reader->Update();

  auto image = reader->GetOutput();
  image->Print(std::cout);

  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion().GetSize(), requestedRegion.GetSize());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion().GetIndex(), requestedRegion.GetIndex());

  writeOutputImage(image, outputPrefix, RESOLUTION);

  return EXIT_SUCCESS;
}

bool
testTimeAndChannelSlice(const std::string & outputPrefix)
{
  // Read a subregion of an arbitrary channel and time point from a 5D image buffer into a 3D image
  using ImageType = itk::Image<unsigned char, 3>;
  const std::string       resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/tczyx.ome.zarr";
  static constexpr size_t RESOLUTION = 2;
  static constexpr size_t TIME_INDEX = 0;
  static constexpr size_t CHANNEL_INDEX = 0;

  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(RESOLUTION);
  imageIO->SetTimeIndex(TIME_INDEX);
  imageIO->SetChannelIndex(CHANNEL_INDEX);

  auto requestedRegion = itk::ImageRegion<3>();
  requestedRegion.SetSize(itk::MakeSize(10, 20, 30));
  requestedRegion.SetIndex(itk::MakeIndex(5, 10, 15));

  // Set up reader
  auto reader = itk::ImageFileReader<ImageType>::New();
  reader->SetFileName(resourceURL);
  reader->SetImageIO(imageIO);
  reader->GetOutput()->SetRequestedRegion(requestedRegion);

  // Read
  reader->Update();

  auto image = reader->GetOutput();
  image->Print(std::cout);

  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion().GetSize(), requestedRegion.GetSize());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion().GetIndex(), requestedRegion.GetIndex());

  writeOutputImage(image, outputPrefix, RESOLUTION);

  return EXIT_SUCCESS;
}

} // namespace

int
itkOMEZarrNGFFHTTPTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " <test-case-id> <outputPrefix>" << std::endl;
    return EXIT_FAILURE;
  }
  size_t            testCase = std::atoi(argv[1]);
  const std::string outputPrefix = argv[2];

  switch (testCase)
  {
    case 0:
      return test2DImage(outputPrefix);
      break;
    case 1:
      return test3DImage(outputPrefix);
      break;
    case 2:
      return testTimeSlice(outputPrefix);
    case 3:
      return testTimeAndChannelSlice(outputPrefix);
    default:
      throw std::invalid_argument("Invalid test case ID: " + std::to_string(testCase));
  }

  return EXIT_FAILURE;
}
