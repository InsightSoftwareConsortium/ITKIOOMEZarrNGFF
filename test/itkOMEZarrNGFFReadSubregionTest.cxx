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

#include <fstream>
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkExtractImageFilter.h"
#include "itkOMEZarrNGFFImageIO.h"
#include "itkOMEZarrNGFFImageIOFactory.h"
#include "itkTestingMacros.h"
#include "itkImageIOBase.h"

int
itkOMEZarrNGFFReadSubregionTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " Input Output" << std::endl;
    return EXIT_FAILURE;
  }
  const char * inputFileName = argv[1];
  const char * outputZarrFileName = argv[2];
  const char * outputMetaImageFileName = argv[3];

  // Set up to request an arbitrary rectangular subregion
  const auto requestedIndex = itk::MakeIndex(32, 64);
  const auto requestedSize = itk::MakeSize(64, 128);

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  // Set up the test by reading input data as MetaImage and constructing
  // local OME-Zarr store for reference
  using ImageType = itk::Image<unsigned char, 2>;
  auto fullImage = itk::ReadImage<ImageType>(inputFileName);
  itk::WriteImage(fullImage, outputZarrFileName);

  // Read in an image subregion with ITKIOOMEZarrNGFF
  auto imageReader = itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(outputZarrFileName);

  using RegionType = typename ImageType::RegionType;
  RegionType requestedRegion;
  requestedRegion.SetIndex(requestedIndex);
  requestedRegion.SetSize(requestedSize);
  imageReader->GetOutput()->SetRequestedRegion(requestedRegion);
  imageReader->Update();

  auto readerOutput = imageReader->GetOutput();
  ITK_TEST_EXPECT_EQUAL(readerOutput->GetRequestedRegion(), requestedRegion);
  ITK_TEST_EXPECT_EQUAL(readerOutput->GetBufferedRegion(), requestedRegion);
  ITK_TEST_EXPECT_EQUAL(readerOutput->GetLargestPossibleRegion(), fullImage->GetLargestPossibleRegion());

  // Limit the largest possible region to the buffered region for write
  auto extractFilter = itk::ExtractImageFilter<ImageType, ImageType>::New();
  extractFilter->SetInput(imageReader->GetOutput());
  extractFilter->SetExtractionRegion(imageReader->GetOutput()->GetBufferedRegion());
  extractFilter->Update();

  auto output = extractFilter->GetOutput();
  ITK_TEST_EXPECT_EQUAL(output->GetRequestedRegion(), requestedRegion);
  ITK_TEST_EXPECT_EQUAL(output->GetBufferedRegion(), requestedRegion);
  ITK_TEST_EXPECT_EQUAL(output->GetLargestPossibleRegion(), requestedRegion);

  // Write for baseline comparison and viewing
  output->Print(std::cout);
  itk::WriteImage(output, outputMetaImageFileName);

  // Validate that the buffered region data matches the same region in the "full" image
  using IteratorType = itk::ImageRegionConstIteratorWithIndex<ImageType>;
  IteratorType fullImageIt(fullImage, output->GetLargestPossibleRegion());
  for (fullImageIt.GoToBegin(); !fullImageIt.IsAtEnd(); ++fullImageIt)
  {
    auto index = fullImageIt.GetIndex();
    itkAssertOrThrowMacro(fullImageIt.Get() == output->GetPixel(index), "Pixel value mismatch at index " << index);
  }

  return EXIT_SUCCESS;
}
