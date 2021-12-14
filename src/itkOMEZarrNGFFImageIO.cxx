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

#include "itkOMEZarrNGFFImageIO.h"
#include "itkIOCommon.h"
#include "itksys/SystemTools.hxx"
#include "itkMath.h"
#include "itkIntTypes.h"
#include "itkMetaDataObject.h"

#include <algorithm>
#include <ctime>

namespace itk
{

OMEZarrNGFFImageIO::OMEZarrNGFFImageIO()

{
  this->AddSupportedWriteExtension(".zarr");
  this->AddSupportedWriteExtension(".zr3");
  this->AddSupportedWriteExtension(".zip");

  this->AddSupportedReadExtension(".zarr");
  this->AddSupportedReadExtension(".zr3");
  this->AddSupportedReadExtension(".zip");
}


void
OMEZarrNGFFImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent); // TODO: either add stuff here, or remove this override
}


bool
OMEZarrNGFFImageIO::CanReadFile(const char * filename)
{
  try
  {
    std::ifstream infile;
    this->OpenFileForReading(infile, filename);

    bool canRead = false;
    if (infile.good())
    {
      // header is a 512 byte block
      char buffer[512];
      infile.read(buffer, 512);
      if (!infile.bad())
      {
        int fileType = OMEZarrNGFFImageIO::CheckVersion(buffer);
        canRead = (fileType > 0);
      }
    }

    infile.close();

    return canRead;
  }
  catch (...) // file cannot be opened, access denied etc.
  {
    return false;
  }
}


void
OMEZarrNGFFImageIO::ReadImageInformation()
{
  if (this->m_FileName.empty())
  {
    itkExceptionMacro("FileName has not been set.");
  }

  std::ifstream infile;
  this->OpenFileForReading(infile, this->m_FileName);

  // header is a 512 byte block
  this->m_RawHeader = new char[512];
  infile.read(this->m_RawHeader, 512);
  int           fileType = 0;
  unsigned long bytesRead = 0;
  if (!infile.bad())
  {
    bytesRead = static_cast<unsigned long>(infile.gcount());
    fileType = OMEZarrNGFFImageIO::CheckVersion(this->m_RawHeader);
  }

  if (fileType == 0)
  {
    infile.close();
    itkExceptionMacro("Unrecognized header in: " << m_FileName);
  }

  if (fileType == 1)
  {
    this->ReadISQHeader(&infile, bytesRead);
  }
  else
  {
    this->ReadAIMHeader(&infile, bytesRead);
  }

  infile.close();

  // This code causes rescaling to Hounsfield units
  if (this->m_MuScaling > 1.0 && this->m_MuWater > 0)
  {
    // mu(voxel) = intensity(voxel) / m_MuScaling
    // HU(voxel) = mu(voxel) * 1000/m_MuWater - 1000
    // Or, HU(voxel) = intensity(voxel) * (1000 / m_MuWater * m_MuScaling) - 1000
    this->m_RescaleSlope = 1000.0 / (this->m_MuWater * this->m_MuScaling);
    this->m_RescaleIntercept = -1000.0;
  }

  this->PopulateMetaDataDictionary();
}

void
OMEZarrNGFFImageIO::PopulateMetaDataDictionary()
{
  MetaDataDictionary & thisDic = this->GetMetaDataDictionary();
  EncapsulateMetaData<std::string>(thisDic, "Version", std::string(this->m_Version));
  EncapsulateMetaData<std::string>(thisDic, "PatientName", std::string(this->m_PatientName));
  EncapsulateMetaData<int>(thisDic, "PatientIndex", this->m_PatientIndex);
  EncapsulateMetaData<int>(thisDic, "ScannerID", this->m_ScannerID);
  EncapsulateMetaData<double>(thisDic, "SliceThickness", this->m_SliceThickness);
  EncapsulateMetaData<double>(thisDic, "SliceIncrement", this->m_SliceIncrement);
}


void
OMEZarrNGFFImageIO::Read(void * buffer)
{
  std::ifstream infile;
  this->OpenFileForReading(infile, this->m_FileName);

  // Dimensions of the data
  const int xsize = this->GetDimensions(0);
  const int ysize = this->GetDimensions(1);
  const int zsize = this->GetDimensions(2);
  size_t    outSize = xsize;
  outSize *= ysize;
  outSize *= zsize;
  outSize *= this->GetComponentSize();

  // ...

  IOComponentEnum dataType = this->m_ComponentType;

  size_t bufferSize = outSize;

  if (this->m_RescaleSlope != 1.0 || this->m_RescaleIntercept != 0.0)
  {
    switch (dataType)
    {
      case IOComponentEnum::CHAR:
        RescaleToHU(reinterpret_cast<char *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::UCHAR:
        RescaleToHU(
          reinterpret_cast<unsigned char *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::SHORT:
        bufferSize /= 2;
        RescaleToHU(reinterpret_cast<short *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::USHORT:
        bufferSize /= 2;
        RescaleToHU(
          reinterpret_cast<unsigned short *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::INT:
        bufferSize /= 4;
        RescaleToHU(reinterpret_cast<int *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::UINT:
        bufferSize /= 4;
        RescaleToHU(
          reinterpret_cast<unsigned int *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      case IOComponentEnum::FLOAT:
        bufferSize /= 4;
        RescaleToHU(reinterpret_cast<float *>(buffer), bufferSize, this->m_RescaleSlope, this->m_RescaleIntercept);
        break;
      default:
        itkExceptionMacro("Unrecognized data type in file: " << dataType);
    }
  }
}


bool
OMEZarrNGFFImageIO::CanWriteFile(const char * name)
{
  const std::string filename = name;

  if (filename.empty())
  {
    return false;
  }

  std::string filenameLower = filename;
  std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);
  std::string::size_type isqPos = filenameLower.rfind(".isq");
  if ((isqPos != std::string::npos) && (isqPos == filename.length() - 4))
  {
    return true;
  }

  return false;
}


void
OMEZarrNGFFImageIO::WriteImageInformation()
{
  if (this->m_FileName.empty())
  {
    itkExceptionMacro("FileName has not been set.");
  }

  std::ofstream outFile;
  this->OpenFileForWriting(outFile, m_FileName);

  this->WriteISQHeader(&outFile);

  outFile.close();
}


void
OMEZarrNGFFImageIO::Write(const void * buffer)
{
  this->WriteImageInformation();

  std::ofstream outFile;
  this->OpenFileForWriting(outFile, m_FileName, false);
  outFile.seekp(this->m_HeaderSize, std::ios::beg);

  const auto numberOfBytes = static_cast<SizeValueType>(this->GetImageSizeInBytes());
  const auto numberOfComponents = static_cast<SizeValueType>(this->GetImageSizeInComponents());

  if (this->GetComponentType() != IOComponentEnum::SHORT)
  {
    itkExceptionMacro("OMEZarrNGFFImageIO only supports writing short files.");
  }

  if (ByteSwapper<short>::SystemIsBigEndian())
  {
    char * tempmemory = new char[numberOfBytes];
    memcpy(tempmemory, buffer, numberOfBytes);
    {
      ByteSwapper<short>::SwapRangeFromSystemToBigEndian(reinterpret_cast<short *>(tempmemory), numberOfComponents);
    }

    // Write the actual pixel data
    outFile.write(static_cast<const char *>(tempmemory), numberOfBytes);
    delete[] tempmemory;
  }
  else
  {
    outFile.write(static_cast<const char *>(buffer), numberOfBytes);
  }

  outFile.close();
}

} // end namespace itk
