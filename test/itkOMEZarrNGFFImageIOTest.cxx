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
#include "itkImageIOBase.h"

#define SPECIFIC_IMAGEIO_MODULE_TEST

template <typename PixelType, unsigned Dimension>
int
doTest(const char * inputFileName, const char * outputFileName)
{
  using ImageType = itk::Image<PixelType, Dimension>;

  using ReaderType = itk::ImageFileReader<ImageType>;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inputFileName);

  // we will need to force use of zarrIO for either reading or writing
  itk::OMEZarrNGFFImageIO::Pointer zarrIO = itk::OMEZarrNGFFImageIO::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(zarrIO, OMEZarrNGFFImageIO, ImageIOBase);

  // check usability of dimension (for coverage)
  if (!zarrIO->SupportsDimension(3))
  {
    std::cerr << "Did not support dimension 3" << std::endl;
    return EXIT_FAILURE;
  }

  if (zarrIO->CanReadFile(inputFileName))
  {
    reader->SetImageIO(zarrIO);
  }
  ITK_TRY_EXPECT_NO_EXCEPTION(reader->Update());

  ImageType::Pointer image = reader->GetOutput();
  image->Print(std::cout);


  using WriterType = itk::ImageFileWriter<ImageType>;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput(reader->GetOutput());
  writer->SetFileName(outputFileName);

  if (zarrIO->CanWriteFile(outputFileName))
  {
    writer->SetImageIO(zarrIO);
  }
  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());

  std::cout << "Test finished" << std::endl;
  return EXIT_SUCCESS;
}

template <typename PixelType>
int
doTest(const char * inputFileName, const char * outputFileName, unsigned dimension)
{
  switch (dimension)
  {
    case 2:
      return doTest<PixelType, 2>(inputFileName, outputFileName);
    case 3:
      return doTest<PixelType, 3>(inputFileName, outputFileName);
    case 4:
      return doTest<PixelType, 4>(inputFileName, outputFileName);
    default:
      itkGenericExceptionMacro("Unsupported image dimension: " << dimension);
      break;
  }
}

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

  using ImageType = itk::Image<unsigned char, 3>;
  auto imageReader = itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(inputFileName);
  imageReader->UpdateOutputInformation();

  unsigned dim = imageReader->GetImageIO()->GetNumberOfDimensions();
  auto     componentType = imageReader->GetImageIO()->GetComponentType();

  switch (componentType)
  {
    case itk::ImageIOBase::IOComponentEnum::UCHAR:
      return doTest<unsigned char>(inputFileName, outputFileName, dim);
      break;
    case itk::ImageIOBase::IOComponentEnum::CHAR:
      return doTest<char>(inputFileName, outputFileName, dim);
      break;
    case itk::ImageIOBase::IOComponentEnum::USHORT:
      return doTest<unsigned short>(inputFileName, outputFileName, dim);
      break;
    case itk::ImageIOBase::IOComponentEnum::SHORT:
      return doTest<short>(inputFileName, outputFileName, dim);
      break;
    case itk::ImageIOBase::IOComponentEnum::FLOAT:
      return doTest<float>(inputFileName, outputFileName, dim);
      break;
    default:
      std::cerr << "Unsupported input image pixel component type: ";
      std::cerr << itk::ImageIOBase::GetComponentTypeAsString(componentType);
      std::cerr << std::endl;
      return EXIT_FAILURE;
      break;
  }
}