/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include <fstream>
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkOMEZarrNGFFImageIO.h"
#include "itkTestingMacros.h"


#define SPECIFIC_IMAGEIO_MODULE_TEST

int
itkOMEZarrNGFFImageIOTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " Input Output" << std::endl;
    return EXIT_FAILURE;
  }
  const char * inputFileName = argv[1];
  const char * outputFileName = argv[2];

  constexpr unsigned int Dimension = 3;
  using PixelType = short;
  using ImageType = itk::Image<PixelType, Dimension>;

  using ReaderType = itk::ImageFileReader<ImageType>;
  ReaderType::Pointer reader = ReaderType::New();

  // force use of zarrIO
  using IOType = itk::OMEZarrNGFFImageIO;
  IOType::Pointer zarrIO = IOType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(zarrIO, OMEZarrNGFFImageIO, ImageIOBase);


  reader->SetImageIO(zarrIO);

  // check usability of dimension (for coverage)
  if (!zarrIO->SupportsDimension(3))
  {
    std::cerr << "Did not support dimension 3" << std::endl;
    return EXIT_FAILURE;
  }

  // read the file
  reader->SetFileName(inputFileName);
  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject & error)
  {
    std::cerr << "Exception in the file reader " << std::endl;
    std::cerr << error << std::endl;
    if (argc == 3) // should fail
    {
      return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
  }

  ImageType::Pointer image = reader->GetOutput();
  image->Print(std::cout);

  ImageType::RegionType region = image->GetLargestPossibleRegion();
  std::cout << "region " << region;

  // Generate test image
  using WriterType = itk::ImageFileWriter<ImageType>;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput(reader->GetOutput());
  writer->SetFileName(outputFileName);
  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());


  std::cout << "Test finished" << std::endl;
  return EXIT_SUCCESS;
}
