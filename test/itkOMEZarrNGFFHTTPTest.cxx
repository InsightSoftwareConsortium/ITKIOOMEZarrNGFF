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
bool
test2DImage()
{
  using ImageType = itk::Image<unsigned char, 2>;
  const std::string resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/yx.ome.zarr";

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  // Baseline image metadata
  auto baselineImage0 = ImageType::New();
  baselineImage0->SetRegions(itk::MakeSize(1024, 930));
  // unit origin, spacing, direction

  auto baselineImage1 = ImageType::New();
  baselineImage1->SetRegions(itk::MakeSize(512, 465));
  const float spacing1[2] = { 2.0, 2.0 };
  baselineImage1->SetSpacing(spacing1);
  // unit origin, direction

  auto baselineImage2 = ImageType::New();
  baselineImage2->SetRegions(itk::MakeSize(256, 232));
  const float spacing2[2] = { 4.0, 4.0 };
  baselineImage2->SetSpacing(spacing2);
  // unit origin, direction

  // Resolution 0
  auto image = itk::ReadImage<ImageType>(resourceURL);
  image->Print(std::cout);

  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage0->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage0->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage0->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage0->GetOrigin());

  // Resolution 1
  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(1);
  auto reader1 = itk::ImageFileReader<ImageType>::New();
  reader1->SetFileName(resourceURL);
  reader1->SetImageIO(imageIO);
  reader1->Update();
  image = reader1->GetOutput();
  image->Print(std::cout);
  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage1->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage1->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage1->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage1->GetOrigin());

  // Resolution 2
  imageIO->SetDatasetIndex(2);
  auto reader2 = itk::ImageFileReader<ImageType>::New();
  reader2->SetFileName(resourceURL);
  reader2->SetImageIO(imageIO);
  reader2->Update();
  image = reader2->GetOutput();
  image->Print(std::cout);
  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage2->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage2->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage2->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage2->GetOrigin());

  return EXIT_SUCCESS;
}

bool
test3DImage()
{
  using ImageType = itk::Image<unsigned char, 3>;
  const std::string resourceURL = "https://s3.embl.de/i2k-2020/ngff-example-data/v0.4/zyx.ome.zarr";

  // Baseline image metadata
  auto baselineImage0 = ImageType::New();
  baselineImage0->SetRegions(itk::MakeSize(483, 393, 603));
  const float spacing0[3] = { 64, 64, 64 };
  baselineImage0->SetSpacing(spacing0);

  auto baselineImage1 = ImageType::New();
  baselineImage1->SetRegions(itk::MakeSize(242, 196, 302));
  const float spacing1[3] = { 128, 128, 128 };
  baselineImage1->SetSpacing(spacing1);

  auto baselineImage2 = ImageType::New();
  baselineImage2->SetRegions(itk::MakeSize(121, 98, 151));
  const float spacing2[3] = { 256, 256, 256 };
  baselineImage2->SetSpacing(spacing2);

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  // Resolution 0
  auto image = itk::ReadImage<ImageType>(resourceURL);
  image->Print(std::cout);

  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage0->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage0->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage0->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage0->GetOrigin());

  // Resolution 1
  auto imageIO = itk::OMEZarrNGFFImageIO::New();
  imageIO->SetDatasetIndex(1);
  auto reader1 = itk::ImageFileReader<ImageType>::New();
  reader1->SetFileName(resourceURL);
  reader1->SetImageIO(imageIO);
  reader1->Update();
  image = reader1->GetOutput();
  image->Print(std::cout);
  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage1->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage1->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage1->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage1->GetOrigin());

  // Resolution 2
  imageIO->SetDatasetIndex(2);
  auto reader2 = itk::ImageFileReader<ImageType>::New();
  reader2->SetFileName(resourceURL);
  reader2->SetImageIO(imageIO);
  reader2->Update();
  image = reader2->GetOutput();
  image->Print(std::cout);
  ITK_TEST_EXPECT_EQUAL(image->GetLargestPossibleRegion(), baselineImage2->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetBufferedRegion(), baselineImage2->GetLargestPossibleRegion());
  ITK_TEST_EXPECT_EQUAL(image->GetSpacing(), baselineImage2->GetSpacing());
  ITK_TEST_EXPECT_EQUAL(image->GetOrigin(), baselineImage2->GetOrigin());

  return EXIT_SUCCESS;
}
} // namespace

int
itkOMEZarrNGFFHTTPTest(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " <test-case-id>" << std::endl;
    return EXIT_FAILURE;
  }
  size_t testCase = std::atoi(argv[1]);

  switch (testCase)
  {
    case 0:
      return test2DImage();
      break;
    case 1:
      return test3DImage();
      break;
    default:
      throw std::invalid_argument("Invalid test case ID: " + std::to_string(testCase));
  }

  return EXIT_FAILURE;
}
