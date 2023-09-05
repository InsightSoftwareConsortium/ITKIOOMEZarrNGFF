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
#include <string>
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkOMEZarrNGFFImageIO.h"
#include "itkOMEZarrNGFFImageIOFactory.h"
#include "itkTestingMacros.h"
#include "itkImageIOBase.h"
#include "itkTestingComparisonImageFilter.h"


namespace
{

template <typename PixelType, unsigned Dimension>
int
doTest(const char * inputFileName, const char * outputFileName)
{
  using ImageType = itk::Image<PixelType, Dimension>;

  //// Read the image from .zip as a baseline
  using ReaderType = itk::ImageFileReader<ImageType>;
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(inputFileName);

  itk::OMEZarrNGFFImageIO::Pointer zarrIO = itk::OMEZarrNGFFImageIO::New();
  reader->SetImageIO(zarrIO);
  ITK_TRY_EXPECT_NO_EXCEPTION(reader->Update());
  typename ImageType::Pointer image = reader->GetOutput();
  image->DisconnectPipeline(); // this is our baseline image

  //// Demonstrate reading from a raw memory buffer

  // Copy zip data into memory
  // Adapted from https://stackoverflow.com/a/18816228/276168
  std::ifstream   file(inputFileName, std::ios::binary | std::ios::ate);
  const std::streamsize inputBufferSize = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<char> buffer(inputBufferSize);
  if (!file.read(buffer.data(), inputBufferSize))
  {
    itkGenericExceptionMacro(<< "Could not read the input file directly: " << inputFileName);
  }
  file.close();

  // Read the zip-compressed bitstream from memory into an ITK image
  itk::OMEZarrNGFFImageIO::BufferInfo bufferInfo{ buffer.data(), buffer.size() };
  const auto inputBufferPointer = buffer.data();
  std::string memAddress = itk::OMEZarrNGFFImageIO::MakeMemoryFileName(bufferInfo);

  typename ReaderType::Pointer memReader = ReaderType::New();
  memReader->SetFileName(memAddress);
  memReader->SetImageIO(itk::OMEZarrNGFFImageIO::New());
  ITK_TRY_EXPECT_NO_EXCEPTION(memReader->Update());
  typename ImageType::Pointer memImage = memReader->GetOutput();

  // Validate both read methods returned the same image
  using CompareType = itk::Testing::ComparisonImageFilter<ImageType, ImageType>;
  typename CompareType::Pointer comparer = CompareType::New();
  comparer->SetValidInput(image);
  comparer->SetTestInput(memImage);
  comparer->Update();
  if (comparer->GetNumberOfPixelsWithDifferences() > 0)
  {
    itkGenericExceptionMacro("The image read through memory is different from the one read through file");
  }

  // Verify a local copy of the buffer is maintained in the itk::Image
  memImage->DisconnectPipeline(); // keep our local copy of the memory buffer
  std::fill(buffer.begin(), buffer.end(), 0);   // reset the memory buffer in place
  ITK_TRY_EXPECT_NO_EXCEPTION(comparer->Update()); // should not reflect buffer update
  if (comparer->GetNumberOfPixelsWithDifferences() > 0)
  {
    itkGenericExceptionMacro("After deallocating the memory buffer, the image read through memory is different from "
                             "the one read through file");
  }

  // Verify our memory buffer is still valid
  ITK_TEST_EXPECT_EQUAL(bufferInfo.pointer, buffer.data());
  ITK_TEST_EXPECT_EQUAL(bufferInfo.size, buffer.size());

  // Write image back to in-memory buffer as a zip compressed bitstream
  using WriterType = itk::ImageFileWriter<ImageType>;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetInput(image);
  writer->SetFileName(memAddress);
  writer->SetImageIO(zarrIO);
  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());

  // Verify the output buffer occupies a new memory region with the expected size
  // and that the BufferInfo object was updated in place to point to the new output buffer
  ITK_TEST_EXPECT_EQUAL(bufferInfo.size, inputBufferSize);
  ITK_TEST_EXPECT_TRUE(bufferInfo.pointer != inputBufferPointer);
  ITK_TEST_EXPECT_EQUAL(itk::OMEZarrNGFFImageIO::MakeMemoryFileName(bufferInfo), memAddress);

  // Write zip bitstream to disk and free the in-memory block
  std::ofstream oFile(outputFileName, std::ios::binary);
  oFile.write(bufferInfo.pointer, bufferInfo.size);
  oFile.close();
  free(bufferInfo.pointer);

  // Validate output file was written and is available for read
  std::ifstream oFileResult(outputFileName);
  ITK_TEST_EXPECT_TRUE(oFileResult.good());

  // Validate output file can be read back in
  auto outputImage = itk::ReadImage<ImageType>(outputFileName);
  outputImage->Print(std::cout);

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
    case 5:
      return doTest<PixelType, 5>(inputFileName, outputFileName);
    default:
      itkGenericExceptionMacro("Unsupported image dimension: " << dimension);
      break;
  }
}
} // namespace

int
itkOMEZarrNGFFInMemoryTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " Input.zip Output.zip" << std::endl;
    return EXIT_FAILURE;
  }
  const char * inputFileName = argv[1];
  const char * outputFileName = argv[2];

  itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();

  using ImageType = itk::Image<unsigned char, 3>;
  auto imageReader = itk::ImageFileReader<ImageType>::New();
  imageReader->SetFileName(inputFileName);
  imageReader->SetImageIO(itk::OMEZarrNGFFImageIO::New()); // explicitly request zarr IO
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
    case itk::ImageIOBase::IOComponentEnum::DOUBLE:
      return doTest<double>(inputFileName, outputFileName, dim);
      break;
    default:
      std::cerr << "Unsupported input image pixel component type: ";
      std::cerr << itk::ImageIOBase::GetComponentTypeAsString(componentType);
      std::cerr << std::endl;
      return EXIT_FAILURE;
      break;
  }
}
