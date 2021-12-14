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
/*=========================================================================

  Program: DICOM for VTK

  Copyright (c) 2015 David Gobbi
  All rights reserved.
  See Copyright.txt or http://dgobbi.github.io/bsd3.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "itkOMEZarrNGFFImageIO.h"
#include "itkSpatialOrientationAdapter.h"
#include "itkIOCommon.h"
#include "itksys/SystemTools.hxx"
#include "itkMath.h"
#include "itkIntTypes.h"
#include "itkByteSwapper.h"
#include "itkMetaDataObject.h"

#include <algorithm>
#include <ctime>

namespace itk
{

OMEZarrNGFFImageIO::OMEZarrNGFFImageIO()

{
  this->m_FileType = IOFileEnum::Binary;
  this->m_ByteOrder = IOByteOrderEnum::LittleEndian;

  this->AddSupportedWriteExtension(".isq");
  this->AddSupportedWriteExtension(".rsq");
  this->AddSupportedWriteExtension(".rad");
  this->AddSupportedWriteExtension(".aim");

  this->AddSupportedReadExtension(".isq");
  this->AddSupportedReadExtension(".rsq");
  this->AddSupportedReadExtension(".rad");
  this->AddSupportedReadExtension(".aim");

  this->m_RawHeader = nullptr;
}


OMEZarrNGFFImageIO::~OMEZarrNGFFImageIO()
{
  delete[] this->m_RawHeader;
}


void
OMEZarrNGFFImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}


int
OMEZarrNGFFImageIO::CheckVersion(const char header[16])
{
  int fileType = 0;

  if (strncmp(header, "CTDATA-HEADER_V1", 16) == 0)
  {
    fileType = 1;
  }
  else if (strcmp(header, "AIMDATA_V030   ") == 0)
  {
    fileType = 3;
  }
  else
  {
    int preHeaderSize = OMEZarrNGFFImageIO::DecodeInt(header);
    int imageHeaderSize = OMEZarrNGFFImageIO::DecodeInt(header + 4);
    if (preHeaderSize == 20 && imageHeaderSize == 140)
    {
      fileType = 2;
    }
  }

  return fileType;
}


int
OMEZarrNGFFImageIO::DecodeInt(const void * data)
{
  const auto * cp = static_cast<const unsigned char *>(data);
  return (cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24));
}


void
OMEZarrNGFFImageIO::EncodeInt(int data, void * target)
{
  auto * targetAsUnsignedChar = static_cast<unsigned char *>(target);
  targetAsUnsignedChar[0] = (unsigned char)(data);
  targetAsUnsignedChar[1] = (unsigned char)(data >> 8);
  targetAsUnsignedChar[2] = (unsigned char)(data >> 16);
  targetAsUnsignedChar[3] = (unsigned char)(data >> 24);
}


float
OMEZarrNGFFImageIO::DecodeFloat(const void * data)
{
  const auto * cp = static_cast<const unsigned char *>(data);
  // different ordering and exponent bias than IEEE 754 float
  union
  {
    float        f;
    unsigned int i;
  } v;
  v.i = (cp[0] << 16) | (cp[1] << 24) | cp[2] | (cp[3] << 8);
  return 0.25 * v.f;
}


double
OMEZarrNGFFImageIO::DecodeDouble(const void * data)
{
  // different ordering and exponent bias than IEEE 754 double
  const auto * cp = static_cast<const unsigned char *>(data);
  union
  {
    double   d;
    uint64_t l;
  } v;
  unsigned int l1, l2;
  l1 = (cp[0] << 16) | (cp[1] << 24) | cp[2] | (cp[3] << 8);
  l2 = (cp[4] << 16) | (cp[5] << 24) | cp[6] | (cp[7] << 8);
  v.l = (static_cast<uint64_t>(l1) << 32) | l2;
  return v.d * 0.25;
}


void
OMEZarrNGFFImageIO::DecodeDate(const void * data,
                          int &        year,
                          int &        month,
                          int &        day,
                          int &        hour,
                          int &        minute,
                          int &        second,
                          int &        millis)
{
  // This is the offset between the astronomical "Julian day", which counts
  // days since January 1, 4713BC, and the "VMS epoch", which counts from
  // November 17, 1858:
  const uint64_t julianOffset = 2400001;
  const uint64_t millisPerSecond = 1000;
  const uint64_t millisPerMinute = 60 * 1000;
  const uint64_t millisPerHour = 3600 * 1000;
  const uint64_t millisPerDay = 3600 * 24 * 1000;

  // Read the date as a long integer with units of 1e-7 seconds
  int      d1 = OMEZarrNGFFImageIO::DecodeInt(data);
  int      d2 = OMEZarrNGFFImageIO::DecodeInt(static_cast<const char *>(data) + 4);
  uint64_t tVMS = d1 + (static_cast<uint64_t>(d2) << 32);
  uint64_t time = tVMS / 10000 + julianOffset * millisPerDay;

  int y, m, d;
  int julianDay = static_cast<int>(time / millisPerDay);
  time -= millisPerDay * julianDay;

  // Gregorian calendar starting from October 15, 1582
  // This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
  int ell, n, i, j;
  ell = julianDay + 68569;
  n = (4 * ell) / 146097;
  ell = ell - (146097 * n + 3) / 4;
  i = (4000 * (ell + 1)) / 1461001;
  ell = ell - (1461 * i) / 4 + 31;
  j = (80 * ell) / 2447;
  d = ell - (2447 * j) / 80;
  ell = j / 11;
  m = j + 2 - (12 * ell);
  y = 100 * (n - 49) + i + ell;

  // Return the result
  year = y;
  month = m;
  day = d;
  hour = static_cast<int>(time / millisPerHour);
  time -= hour * millisPerHour;
  minute = static_cast<int>(time / millisPerMinute);
  time -= minute * millisPerMinute;
  second = static_cast<int>(time / millisPerSecond);
  time -= second * millisPerSecond;
  millis = static_cast<int>(time);
}


void
OMEZarrNGFFImageIO::EncodeDate(void * target)
{
  time_t currentTimeUnix;
  std::time(&currentTimeUnix);
  const uint64_t currentTimeVMS = currentTimeUnix * 10000000 + 3506716800;

  const int d1 = (int)currentTimeVMS;
  const int d2 = (int)(currentTimeVMS >> 32);
  OMEZarrNGFFImageIO::EncodeInt(d1, target);
  OMEZarrNGFFImageIO::EncodeInt(d2, static_cast<char *>(target) + 4);
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
OMEZarrNGFFImageIO::InitializeHeader()
{
  memset(this->m_Version, 0, 18);
  memset(this->m_PatientName, 0, 42);
  this->m_PatientIndex = 0;
  this->m_ScannerID = 0;
  memset(this->m_CreationDate, 0, 32);
  memset(this->m_ModificationDate, 0, 32);
  this->ScanDimensionsPixels[0] = 0;
  this->ScanDimensionsPixels[1] = 0;
  this->ScanDimensionsPixels[2] = 0;
  this->ScanDimensionsPhysical[0] = 0;
  this->ScanDimensionsPhysical[1] = 0;
  this->ScanDimensionsPhysical[2] = 0;
  this->m_SliceThickness = 0;
  this->m_SliceIncrement = 0;
  this->m_StartPosition = 0;
  this->m_EndPosition = 0;
  this->m_ZPosition = 0;
  this->m_DataRange[0] = 0;
  this->m_DataRange[1] = 0;
  this->m_MuScaling = 1.0;
  this->m_NumberOfSamples = 0;
  this->m_NumberOfProjections = 0;
  this->m_ScanDistance = 0;
  this->m_SampleTime = 0;
  this->m_ScannerType = 0;
  this->m_MeasurementIndex = 0;
  this->m_Site = 0;
  this->m_ReconstructionAlg = 0;
  this->m_ReferenceLine = 0;
  this->m_Energy = 0;
  this->m_Intensity = 0;

  this->m_RescaleType = 0;
  memset(this->m_RescaleUnits, 0, 18);
  memset(this->m_CalibrationData, 0, 66);
  this->m_RescaleSlope = 1.0;
  this->m_RescaleIntercept = 0.0;
  this->m_MuWater = 0.70329999923706055;

  this->m_Compression = 0;
  this->m_HeaderInitialized = true;
}


void
OMEZarrNGFFImageIO::StripString(char * dest, const char * source, size_t length)
{
  char * dp = dest;
  for (size_t i = 0; i < length && *source != '\0'; ++i)
  {
    *dp++ = *source++;
  }
  while (dp != dest && dp[-1] == ' ')
  {
    --dp;
  }
  *dp = '\0';
}


void
OMEZarrNGFFImageIO::PadString(char * dest, const char * source, size_t length)
{
  for (size_t i = 0; i < length && *source != '\0'; ++i)
  {
    *dest++ = *source++;
  }
  for (size_t i = 0; i < length; ++i)
  {
    *dest++ = ' ';
  }
}


int
OMEZarrNGFFImageIO::ReadISQHeader(std::ifstream * file, unsigned long bytesRead)
{
  if (bytesRead < 512)
  {
    return 0;
  }

  char * h = this->m_RawHeader;
  OMEZarrNGFFImageIO::StripString(this->m_Version, h, 16);
  h += 16;
  int dataType = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  const int numBytes = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  (void)numBytes;
  const int numBlocks = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  (void)numBlocks;
  this->m_PatientIndex = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  this->m_ScannerID = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  int year, month, day, hour, minute, second, milli;
  OMEZarrNGFFImageIO::DecodeDate(h, year, month, day, hour, minute, second, milli);
  h += 8;
  int pixdim[3], physdim[3];
  pixdim[0] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  pixdim[1] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  pixdim[2] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  physdim[0] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  physdim[1] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;
  physdim[2] = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;

  const bool isRAD = (dataType == 9 || physdim[2] == 0);

  if (isRAD) // RAD file
  {
    this->m_MeasurementIndex = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_DataRange[0] = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_DataRange[1] = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_MuScaling = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    OMEZarrNGFFImageIO::StripString(this->m_PatientName, h, 40);
    h += 40;
    this->m_ZPosition = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    /* unknown */ h += 4;
    this->m_SampleTime = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_Energy = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_Intensity = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_ReferenceLine = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_StartPosition = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_EndPosition = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    h += 88 * 4;
  }
  else // ISQ file or RSQ file
  {
    this->m_SliceThickness = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_SliceIncrement = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_StartPosition = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_EndPosition = this->m_StartPosition + physdim[2] * 1e-3 * (pixdim[2] - 1) / pixdim[2];
    this->m_DataRange[0] = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_DataRange[1] = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_MuScaling = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_NumberOfSamples = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_NumberOfProjections = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_ScanDistance = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_ScannerType = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_SampleTime = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_MeasurementIndex = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_Site = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    this->m_ReferenceLine = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_ReconstructionAlg = OMEZarrNGFFImageIO::DecodeInt(h);
    h += 4;
    OMEZarrNGFFImageIO::StripString(this->m_PatientName, h, 40);
    h += 40;
    this->m_Energy = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    this->m_Intensity = OMEZarrNGFFImageIO::DecodeInt(h) * 1e-3;
    h += 4;
    h += 83 * 4;
  }

  int dataOffset = OMEZarrNGFFImageIO::DecodeInt(h);

  // fix m_SliceThickness and m_SliceIncrement if they were truncated
  if (physdim[2] != 0)
  {
    double computedSpacing = physdim[2] * 1e-3 / pixdim[2];
    if (fabs(computedSpacing - this->m_SliceThickness) < 1.1e-3)
    {
      this->m_SliceThickness = computedSpacing;
    }
    if (fabs(computedSpacing - this->m_SliceIncrement) < 1.1e-3)
    {
      this->m_SliceIncrement = computedSpacing;
    }
  }

  // Convert date information into a string
  month = ((month > 12 || month < 1) ? 0 : month);
  static const char * months[] = { "XXX", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
  sprintf(this->m_CreationDate,
          "%d-%s-%d %02d:%02d:%02d.%03d",
          (day % 100),
          months[month],
          (year % 10000),
          (hour % 100),
          (minute % 100),
          (second % 100),
          (milli % 1000));
  sprintf(this->m_ModificationDate,
          "%d-%s-%d %02d:%02d:%02d.%03d",
          (day % 100),
          months[month],
          (year % 10000),
          (hour % 100),
          (minute % 100),
          (second % 100),
          (milli % 1000));

  // Perform a sanity check on the dimensions
  for (int i = 0; i < 3; ++i)
  {
    this->ScanDimensionsPixels[i] = pixdim[i];
    if (pixdim[i] < 1)
    {
      pixdim[i] = 1;
    }
    this->ScanDimensionsPhysical[i] = (isRAD ? physdim[i] * 1e-6 : physdim[i] * 1e-3);
    if (physdim[i] == 0)
    {
      physdim[i] = 1.0;
    }
  }

  this->SetNumberOfDimensions(3);
  for (unsigned int i = 0; i < m_NumberOfDimensions; ++i)
  {
    this->SetDimensions(i, pixdim[i]);
    if (isRAD) // RAD file
    {
      if (i == 2)
      {
        this->SetSpacing(i, 1.0);
      }
      else
      {
        this->SetSpacing(i, physdim[i] * 1e-6 / pixdim[i]);
      }
    }
    else
    {
      this->SetSpacing(i, physdim[i] * 1e-3 / pixdim[i]);
    }
    this->SetOrigin(i, 0.0);
  }

  this->SetPixelType(IOPixelEnum::SCALAR);
  this->SetComponentType(IOComponentEnum::SHORT);

  // total header size
  const SizeValueType headerSize = static_cast<SizeValueType>(dataOffset + 1) * 512;
  this->m_HeaderSize = headerSize;

  // read the rest of the header
  if (headerSize > bytesRead)
  {
    h = new char[headerSize];
    memcpy(h, this->m_RawHeader, bytesRead);
    delete[] this->m_RawHeader;
    this->m_RawHeader = h;
    file->read(h + bytesRead, headerSize - bytesRead);
    if (static_cast<unsigned long>(file->gcount()) < headerSize - bytesRead)
    {
      return 0;
    }
  }

  // decode the extended header (lots of guesswork)
  if (headerSize >= 2048)
  {
    char * calHeader = nullptr;
    int    calHeaderSize = 0;
    h = this->m_RawHeader + 512;
    unsigned long hskip = 1;
    char *        headerName = h + 8;
    if (strncmp(headerName, "MultiHeader     ", 16) == 0)
    {
      h += 512;
      hskip += 1;
    }
    unsigned long hsize = 0;
    for (int i = 0; i < 4; ++i)
    {
      hsize = OMEZarrNGFFImageIO::DecodeInt(h + i * 128 + 24);
      if ((1 + hskip + hsize) * 512 > headerSize)
      {
        break;
      }
      headerName = h + i * 128 + 8;
      if (strncmp(headerName, "Calibration     ", 16) == 0)
      {
        calHeader = this->m_RawHeader + (1 + hskip) * 512;
        calHeaderSize = hsize * 512;
      }
      hskip += hsize;
    }

    if (calHeader && calHeaderSize >= 1024)
    {
      h = calHeader;
      OMEZarrNGFFImageIO::StripString(this->m_CalibrationData, h + 28, 64);
      // std::string calFile(h + 112, 256);
      // std::string s3(h + 376, 256);
      this->m_RescaleType = OMEZarrNGFFImageIO::DecodeInt(h + 632);
      OMEZarrNGFFImageIO::StripString(this->m_RescaleUnits, h + 648, 16);
      // std::string s5(h + 700, 16);
      // std::string calFilter(h + 772, 16);
      this->m_RescaleSlope = OMEZarrNGFFImageIO::DecodeDouble(h + 664);
      this->m_RescaleIntercept = OMEZarrNGFFImageIO::DecodeDouble(h + 672);
      this->m_MuWater = OMEZarrNGFFImageIO::DecodeDouble(h + 688);
    }
  }

  // Include conversion to linear att coeff in the rescaling
  if (this->m_MuScaling > 1.0)
  {
    this->m_RescaleSlope /= this->m_MuScaling;
  }

  return 1;
}


int
OMEZarrNGFFImageIO::ReadAIMHeader(std::ifstream * file, unsigned long bytesRead)
{
  if (bytesRead < 160)
  {
    return 0;
  }

  char *        h = this->m_RawHeader;
  int           intSize = 0;
  unsigned long headerSize = 0;

  // True for AIM v030, False for AIM v020
  bool versionV030 = !strcmp(h, "AIMDATA_V030   ");

  if (versionV030)
  {
    // All header data is saved as 64-bit ints (8 bytes), except for the datatype which is a 32-bit int
    // AIMDATA_V030 data has 16-bits (2 bytes) extra at the front of the header for a string containing "AIMDATA_V030"
    intSize = 8;
    strcpy(this->m_Version, h);
    headerSize = 16;
    h += headerSize;
  }
  else
  {
    // All header data is saved as 32-bit ints (4 bytes), except for element size which is saved as a float
    intSize = 4;
    strcpy(this->m_Version, "AIMDATA_V020   ");
  }

  // Read the pre-header
  // AIM header is divided into 3 sections: a preheader, a struct containing volume info, and a processing log
  char * preheader = h;
  int    preheaderSize = OMEZarrNGFFImageIO::DecodeInt(h);
  h += intSize;
  int structSize = OMEZarrNGFFImageIO::DecodeInt(h);
  h += intSize;
  int logSize = OMEZarrNGFFImageIO::DecodeInt(h);
  h += intSize;

  // read the rest of the header
  headerSize += preheaderSize + structSize + logSize;
  this->m_HeaderSize = headerSize;

  if (headerSize > bytesRead)
  {
    h = new char[headerSize];
    memcpy(h, this->m_RawHeader, bytesRead);
    preheader = h + (preheader - this->m_RawHeader);
    delete[] this->m_RawHeader;
    this->m_RawHeader = h;
    file->read(h + bytesRead, headerSize - bytesRead);
    if (static_cast<unsigned long>(file->gcount()) < headerSize - bytesRead)
    {
      return 0;
    }
  }

  // decode the struct header
  h = preheader + preheaderSize;

  // The datatype value is stored as a 4 byte block for both AIM v020 and v030.
  // There are some extra bytes before the datatype:
  // 3 4-byte blocks for AIMDATA_V030
  // 5 4-byte blocks for AIMDATA_V020
  if (versionV030)
  {
    h += 12;
  }
  else
  {
    h += 20;
  }

  int dataType = OMEZarrNGFFImageIO::DecodeInt(h);
  h += 4;

  // The struct contains the following data:
  // structValues[0] to structValues[2] = image position
  // structValues[3] to structValues[5] = image dimensions
  // structValues[6] to structValues[21] = 15 blocks of zeros
  int structValues[21];
  for (int & structValue : structValues)
  {
    structValue = OMEZarrNGFFImageIO::DecodeInt(h);
    h += intSize;
  }

  // Element Size is float in AIMDATA_V020, but 64-bit int in AIMDATA_V030
  float elementSize[3];
  if (versionV030)
  {
    // AIMDATA_V030
    for (float & i : elementSize)
    {
      i = 1e-6 * OMEZarrNGFFImageIO::DecodeInt(h);
      if (i == 0)
      {
        i = 1.0;
      }
      h += intSize;
    }
  }
  else
  {
    // AIMDATA_V020
    for (float & i : elementSize)
    {
      i = OMEZarrNGFFImageIO::DecodeFloat(h);
      if (i == 0)
      {
        i = 1.0;
      }
      h += intSize;
    }
  }

  // number of components per pixel is 1 by default
  this->SetPixelType(IOPixelEnum::SCALAR);
  this->m_Compression = 0;

  // a limited selection of data types are supported
  // (only 0x00010001 (char) and 0x00020002 (short) are fully tested)
  switch (dataType)
  {
    case 0x00160001:
      this->SetComponentType(IOComponentEnum::UCHAR);
      break;
    case 0x000d0001:
      this->SetComponentType(IOComponentEnum::UCHAR);
      break;
    case 0x00120003:
      this->SetComponentType(IOComponentEnum::UCHAR);
      this->SetPixelType(IOPixelEnum::VECTOR);
      this->SetNumberOfDimensions(3);
      break;
    case 0x00010001:
      this->SetComponentType(IOComponentEnum::CHAR);
      break;
    case 0x00060003:
      this->SetComponentType(IOComponentEnum::CHAR);
      this->SetPixelType(IOPixelEnum::VECTOR);
      this->SetNumberOfDimensions(3);
      break;
    case 0x00170002:
      this->SetComponentType(IOComponentEnum::USHORT);
      break;
    case 0x00020002:
      this->SetComponentType(IOComponentEnum::SHORT);
      break;
    case 0x00030004:
      this->SetComponentType(IOComponentEnum::INT);
      break;
    case 0x001a0004:
      this->SetComponentType(IOComponentEnum::FLOAT);
      break;
    case 0x00150001:
      this->m_Compression = 0x00b2; // run-length compressed bits
      this->SetComponentType(IOComponentEnum::CHAR);
      break;
    case 0x00080002:
      this->m_Compression = 0x00c2; // run-length compressed signed char
      this->SetComponentType(IOComponentEnum::CHAR);
      break;
    case 0x00060001:
      this->m_Compression = 0x00b1; // packed bits
      this->SetComponentType(IOComponentEnum::CHAR);
      break;
    default:
      itkExceptionMacro("Unrecognized data type in AIM file: " << dataType);
  }

  this->SetNumberOfDimensions(3);
  for (unsigned int i = 0; i < m_NumberOfDimensions; ++i)
  {
    this->SetDimensions(i, structValues[3 + i]);
    this->SetSpacing(i, elementSize[i]);
    // the origin will reflect the cropping of the data
    this->SetOrigin(i, elementSize[i] * structValues[i]);
  }

  // decode the processing log
  h = preheader + preheaderSize + structSize;
  char * logEnd = h + logSize;

  while (h != logEnd && *h != '\0')
  {
    // skip newline and go to next line
    if (*h == '\n')
    {
      ++h;
    }

    // search for the end of this line
    char * lineEnd = h;
    while (lineEnd != logEnd && *lineEnd != '\n' && *lineEnd != '\0')
    {
      ++lineEnd;
    }

    // if not a comment, search for keys
    if (h != lineEnd && *h != '!' && (*lineEnd == '\n' || *lineEnd == '\0'))
    {
      // key and value are separated by multiple spaces
      char * key = h;
      while (h + 1 != lineEnd && (h[0] != ' ' || h[1] != ' '))
      {
        ++h;
      }
      // this gives the length of the key
      size_t keylen = h - key;
      // skip to the end of the spaces
      while (h != lineEnd && *h == ' ')
      {
        ++h;
      }
      // this is where the value starts
      char * value = h;
      size_t valuelen = lineEnd - value;
      // look for trailing spaces
      while (valuelen > 0 && (h[valuelen - 1] == ' ' || h[valuelen - 1] == '\r'))
      {
        --valuelen;
      }

      // convert into a std::string for convenience
      std::string skey(key, keylen);

      // check for known keys
      if (skey == "Time")
      {
        valuelen = (valuelen > 31 ? 31 : valuelen);
        strncpy(this->m_ModificationDate, value, valuelen);
        this->m_ModificationDate[valuelen] = '\0';
      }
      else if (skey == "Original Creation-Date")
      {
        valuelen = (valuelen > 31 ? 31 : valuelen);
        strncpy(this->m_CreationDate, value, valuelen);
        this->m_CreationDate[valuelen] = '\0';
      }
      else if (skey == "Orig-ISQ-Dim-p")
      {
        for (int & ScanDimensionsPixel : this->ScanDimensionsPixels)
        {
          ScanDimensionsPixel = strtol(value, &value, 10);
        }
      }
      else if (skey == "Orig-ISQ-Dim-um")
      {
        for (double & i : this->ScanDimensionsPhysical)
        {
          i = strtod(value, &value) * 1e-3;
        }
      }
      else if (skey == "Patient Name")
      {
        valuelen = (valuelen > 41 ? 41 : valuelen);
        strncpy(this->m_PatientName, value, valuelen);
        this->m_PatientName[valuelen] = '\0';
      }
      else if (skey == "Index Patient")
      {
        this->m_PatientIndex = strtol(value, nullptr, 10);
      }
      else if (skey == "Index Measurement")
      {
        this->m_MeasurementIndex = strtol(value, nullptr, 10);
      }
      else if (skey == "Site")
      {
        this->m_Site = strtol(value, nullptr, 10);
      }
      else if (skey == "Scanner ID")
      {
        this->m_ScannerID = strtol(value, nullptr, 10);
      }
      else if (skey == "Scanner type")
      {
        this->m_ScannerType = strtol(value, nullptr, 10);
      }
      else if (skey == "Position Slice 1 [um]")
      {
        this->m_StartPosition = strtod(value, nullptr) * 1e-3;
        this->m_EndPosition = this->m_StartPosition + elementSize[2] * (structValues[5] - 1);
      }
      else if (skey == "No. samples")
      {
        this->m_NumberOfSamples = strtol(value, nullptr, 10);
      }
      else if (skey == "No. projections per 180")
      {
        this->m_NumberOfProjections = strtol(value, nullptr, 10);
      }
      else if (skey == "Scan Distance [um]")
      {
        this->m_ScanDistance = strtod(value, nullptr) * 1e-3;
      }
      else if (skey == "Integration time [us]")
      {
        this->m_SampleTime = strtod(value, nullptr) * 1e-3;
      }
      else if (skey == "Reference line [um]")
      {
        this->m_ReferenceLine = strtod(value, nullptr) * 1e-3;
      }
      else if (skey == "Reconstruction-Alg.")
      {
        this->m_ReconstructionAlg = strtol(value, nullptr, 10);
      }
      else if (skey == "Energy [V]")
      {
        this->m_Energy = strtod(value, nullptr) * 1e-3;
      }
      else if (skey == "Intensity [uA]")
      {
        this->m_Intensity = strtod(value, nullptr) * 1e-3;
      }
      else if (skey == "Mu_Scaling")
      {
        this->m_MuScaling = strtol(value, nullptr, 10);
      }
      else if (skey == "Minimum data value")
      {
        this->m_DataRange[0] = strtod(value, nullptr);
      }
      else if (skey == "Maximum data value")
      {
        this->m_DataRange[1] = strtod(value, nullptr);
      }
      else if (skey == "Calib. default unit type")
      {
        this->m_RescaleType = strtol(value, nullptr, 10);
      }
      else if (skey == "Calibration Data")
      {
        valuelen = (valuelen > 64 ? 64 : valuelen);
        strncpy(this->m_CalibrationData, value, valuelen);
        this->m_CalibrationData[valuelen] = '\0';
      }
      else if (skey == "Density: unit")
      {
        valuelen = (valuelen > 16 ? 16 : valuelen);
        strncpy(this->m_RescaleUnits, value, valuelen);
        this->m_RescaleUnits[valuelen] = '\0';
      }
      else if (skey == "Density: slope")
      {
        this->m_RescaleSlope = strtod(value, nullptr);
      }
      else if (skey == "Density: intercept")
      {
        this->m_RescaleIntercept = strtod(value, nullptr);
      }
      else if (skey == "HU: mu water")
      {
        this->m_MuWater = strtod(value, nullptr);
      }
    }
    // skip to the end of the line
    h = lineEnd;
  }

  // Include conversion to linear att coeff in the rescaling
  if (this->m_MuScaling > 1.0)
  {
    this->m_RescaleSlope /= this->m_MuScaling;
  }

  // these items are not in the processing log
  this->m_SliceThickness = elementSize[2];
  this->m_SliceIncrement = elementSize[2];

  return 1;
}


void
OMEZarrNGFFImageIO::ReadImageInformation()
{
  this->InitializeHeader();

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
  EncapsulateMetaData<std::string>(thisDic, "CreationDate", std::string(this->m_CreationDate));
  EncapsulateMetaData<std::string>(thisDic, "ModificationDate", std::string(this->m_ModificationDate));
  EncapsulateMetaData<double>(thisDic, "SliceThickness", this->m_SliceThickness);
  EncapsulateMetaData<double>(thisDic, "SliceIncrement", this->m_SliceIncrement);
  std::vector<double> dataRange(2);
  dataRange[0] = this->m_DataRange[0];
  dataRange[1] = this->m_DataRange[1];
  EncapsulateMetaData<std::vector<double>>(thisDic, "DataRange", dataRange);
  EncapsulateMetaData<double>(thisDic, "MuScaling", this->m_MuScaling);
  EncapsulateMetaData<int>(thisDic, "NumberOfSamples", this->m_NumberOfSamples);
  EncapsulateMetaData<int>(thisDic, "NumberOfProjections", this->m_NumberOfProjections);
  EncapsulateMetaData<double>(thisDic, "ScanDistance", this->m_ScanDistance);
  EncapsulateMetaData<double>(thisDic, "SampleTime", this->m_SampleTime);
  EncapsulateMetaData<int>(thisDic, "ScannerType", this->m_ScannerType);
  EncapsulateMetaData<int>(thisDic, "MeasurementIndex", this->m_MeasurementIndex);
  EncapsulateMetaData<int>(thisDic, "Site", this->m_Site);
  EncapsulateMetaData<int>(thisDic, "ReconstructionAlg", this->m_ReconstructionAlg);
  EncapsulateMetaData<double>(thisDic, "ReferenceLine", this->m_ReferenceLine);
  EncapsulateMetaData<double>(thisDic, "Energy", this->m_Energy);
  EncapsulateMetaData<double>(thisDic, "Intensity", this->m_Intensity);
  EncapsulateMetaData<int>(thisDic, "RescaleType", this->m_RescaleType);
  EncapsulateMetaData<std::string>(thisDic, "RescaleUnits", std::string(this->m_RescaleUnits));
  EncapsulateMetaData<std::string>(thisDic, "CalibrationData", std::string(this->m_CalibrationData));
  EncapsulateMetaData<double>(thisDic, "RescaleSlope", this->m_RescaleSlope);
  EncapsulateMetaData<double>(thisDic, "RescaleIntercept", this->m_RescaleIntercept);
  EncapsulateMetaData<double>(thisDic, "MuWater", this->m_MuWater);
}

void
OMEZarrNGFFImageIO::SetHeaderFromMetaDataDictionary()
{
  MetaDataDictionary & metaData = this->GetMetaDataDictionary();

  std::string stringMeta;
  if (ExposeMetaData<std::string>(metaData, "Version", stringMeta))
  {
    strncpy(this->m_Version, stringMeta.c_str(), 18);
  }
  if (ExposeMetaData<std::string>(metaData, "PatientName", stringMeta))
  {
    strncpy(this->m_PatientName, stringMeta.c_str(), 42);
  }

  ExposeMetaData<int>(metaData, "PatientIndex", this->m_PatientIndex);
  ExposeMetaData<int>(metaData, "ScannerID", this->m_ScannerID);

  if (ExposeMetaData<std::string>(metaData, "CreationDate", stringMeta))
  {
    strncpy(this->m_CreationDate, stringMeta.c_str(), 32);
  }
  if (ExposeMetaData<std::string>(metaData, "ModificationDate", stringMeta))
  {
    strncpy(this->m_ModificationDate, stringMeta.c_str(), 32);
  }

  ExposeMetaData<double>(metaData, "SliceThickness", this->m_SliceThickness);
  ExposeMetaData<double>(metaData, "SliceIncrement", this->m_SliceIncrement);

  std::vector<double> dataRange(2);
  if (ExposeMetaData<std::vector<double>>(metaData, "DataRange", dataRange))
  {
    this->m_DataRange[0] = dataRange[0];
    this->m_DataRange[1] = dataRange[1];
  }

  ExposeMetaData<double>(metaData, "MuScaling", this->m_MuScaling);
  ExposeMetaData<int>(metaData, "NumberOfSamples", this->m_NumberOfSamples);
  ExposeMetaData<int>(metaData, "NumberOfProjections", this->m_NumberOfProjections);
  ExposeMetaData<double>(metaData, "ScanDistance", this->m_ScanDistance);
  ExposeMetaData<double>(metaData, "SampleTime", this->m_SampleTime);
  ExposeMetaData<int>(metaData, "ScannerType", this->m_ScannerType);
  ExposeMetaData<int>(metaData, "MeasurementIndex", this->m_MeasurementIndex);
  ExposeMetaData<int>(metaData, "Site", this->m_Site);
  ExposeMetaData<int>(metaData, "ReconstructionAlg", this->m_ReconstructionAlg);
  ExposeMetaData<double>(metaData, "ReferenceLine", this->m_ReferenceLine);
  ExposeMetaData<double>(metaData, "Energy", this->m_Energy);
  ExposeMetaData<double>(metaData, "Intensity", this->m_Intensity);
  ExposeMetaData<double>(metaData, "Intensity", this->m_Intensity);

  ExposeMetaData<int>(metaData, "RescaleType", this->m_RescaleType);
  if (ExposeMetaData<std::string>(metaData, "RescaleUnits", stringMeta))
  {
    strncpy(this->m_RescaleUnits, stringMeta.c_str(), 18);
  }
  if (ExposeMetaData<std::string>(metaData, "CalibrationData", stringMeta))
  {
    strncpy(this->m_CalibrationData, stringMeta.c_str(), 66);
  }

  ExposeMetaData<double>(metaData, "RescaleSlope", this->m_RescaleSlope);
  ExposeMetaData<double>(metaData, "RescaleIntercept", this->m_RescaleIntercept);
  ExposeMetaData<double>(metaData, "MuWater", this->m_MuWater);
}

template <typename TBufferType>
void
RescaleToHU(TBufferType * buffer, size_t size, double slope, double intercept)
{
  for (size_t i = 0; i < size; i++)
  {
    float bufferValue = static_cast<float>(buffer[i]);
    bufferValue = bufferValue * slope + intercept;
    buffer[i] = static_cast<TBufferType>(bufferValue);
  }
}

void
OMEZarrNGFFImageIO::Read(void * buffer)
{
  std::ifstream infile;
  this->OpenFileForReading(infile, this->m_FileName);

  // seek to the data
  infile.seekg(this->m_HeaderSize);

  // get the size of the compressed data
  int intSize = 4;
  if (strcmp(this->m_Version, "AIMDATA_V030   ") == 0)
  {
    // header uses 64-bit ints (8 bytes)
    intSize = 8;
  }

  // Dimensions of the data
  const int xsize = this->GetDimensions(0);
  const int ysize = this->GetDimensions(1);
  const int zsize = this->GetDimensions(2);
  size_t    outSize = xsize;
  outSize *= ysize;
  outSize *= zsize;
  outSize *= this->GetComponentSize();

  // For the input (compressed) data
  char * input = nullptr;
  size_t size = 0;

  if (this->m_Compression == 0)
  {
    infile.read(reinterpret_cast<char *>(buffer), outSize);
    size = outSize;
  }
  else if (this->m_Compression == 0x00b1)
  {
    // Compute the size of the binary packed data
    size_t xinc = (xsize + 1) / 2;
    size_t yinc = (ysize + 1) / 2;
    size_t zinc = (zsize + 1) / 2;
    size = xinc * yinc * zinc + 1;
    input = new char[size];
    infile.read(input, size);
  }
  else if (this->m_Compression == 0x00b2 || this->m_Compression == 0x00c2)
  {
    // Get the size of the compressed data
    char head[8];
    infile.read(head, intSize);
    size = static_cast<unsigned int>(OMEZarrNGFFImageIO::DecodeInt(head));
    if (intSize == 8)
    {
      // Read the high word of a 64-bit int
      unsigned int high = OMEZarrNGFFImageIO::DecodeInt(head + 4);
      size += (static_cast<uint64_t>(high) << 32);
    }
    input = new char[size - intSize];
    size -= intSize;
    infile.read(input, size);
  }

  // confirm that enough data was read
  size_t shortread = size - infile.gcount();
  if (shortread != 0)
  {
    itkExceptionMacro("File is truncated, " << shortread << " bytes are missing");
  }

  // Close the file
  infile.close();

  auto * dataPtr = reinterpret_cast<unsigned char *>(buffer);

  if (this->m_Compression == 0x00b1)
  {
    // Unpack binary data, each byte becomes a 2x2x2 block of voxels
    size_t        xinc = (xsize + 1) / 2;
    size_t        yinc = (ysize + 1) / 2;
    unsigned char v = input[size - 1];
    v = (v == 0 ? 0x7f : v);
    unsigned char bit = 0;
    for (int i = 0; i < zsize; i++)
    {
      bit ^= (bit & 2);
      for (int j = 0; j < ysize; j++)
      {
        char * inPtr = input + (i * yinc + j) * xinc;
        bit ^= (bit & 1);
        for (int k = 0; k < xsize; k++)
        {
          unsigned char c = *inPtr;
          *dataPtr++ = ((c >> bit) & 1) * v;
          inPtr += (bit & 1);
          bit ^= 1;
        }
        bit ^= 2;
      }
      bit ^= 4;
    }
  }
  else if (this->m_Compression == 0x00b2)
  {
    // Decompress binary run-lengths
    bool          flip = false;
    unsigned char v = input[flip];
    char *        inPtr = input + 2;
    size -= 2;
    if (size > 0)
    {
      do
      {
        unsigned char l = *inPtr++;
        if (l == 255)
        {
          l = 254;
          flip = !flip;
        }
        if (l > outSize)
        {
          l = static_cast<unsigned char>(outSize);
        }
        outSize -= l;
        if (l > 0)
        {
          do
          {
            *dataPtr++ = v;
          } while (--l);
        }
        flip = !flip;
        v = input[flip];
      } while (--size != 0 && outSize != 0);
    }
  }
  else if (this->m_Compression == 0x00c2)
  {
    // Decompress 8-bit run-lengths
    char * inPtr = input;
    size /= 2;
    if (size > 0)
    {
      do
      {
        unsigned char l = inPtr[0];
        unsigned char v = inPtr[1];
        inPtr += 2;
        if (l > outSize)
        {
          l = static_cast<unsigned char>(outSize);
        }
        outSize -= l;
        if (l > 0)
        {
          do
          {
            *dataPtr++ = v;
          } while (--l);
        }
      } while (--size != 0 && outSize != 0);
    }
  }

  delete[] input;

  // Convert the image to HU.
  // Only SHORT images have been tested
  IOComponentEnum dataType = this->m_ComponentType;

  // The size of the buffer will change depending on the data type
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
OMEZarrNGFFImageIO::WriteISQHeader(std::ofstream * file)
{
  if (!this->m_HeaderInitialized)
  {
    this->InitializeHeader();
  }
  this->SetHeaderFromMetaDataDictionary();
  // now overwrite some values which we don't want taken from metadata
  this->SetVersion("CTDATA-HEADER_V1");
  this->m_MuScaling = 1.0; // we don't want rescaling to occur on reading

  delete[] this->m_RawHeader;
  this->m_RawHeader = new char[512];
  char * header = this->m_RawHeader;

  OMEZarrNGFFImageIO::PadString(header, this->m_Version, 16);
  header += 16;
  // 3 -> ISQ data type
  OMEZarrNGFFImageIO::EncodeInt(3, header);
  header += 4;
  const auto numberOfBytes = static_cast<SizeValueType>(this->GetImageSizeInBytes());
  if (numberOfBytes > NumericTraits<int>::max())
  {
    OMEZarrNGFFImageIO::EncodeInt(0, header);
    header += 4;
  }
  else
  {
    OMEZarrNGFFImageIO::EncodeInt(numberOfBytes, header);
    header += 4;
  }
  OMEZarrNGFFImageIO::EncodeInt(numberOfBytes / 512, header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt(this->m_PatientIndex, header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt(this->m_ScannerID, header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeDate(header);
  header += 8;
  for (unsigned int dimension = 0; dimension < 3; ++dimension)
  {
    // pixdim
    OMEZarrNGFFImageIO::EncodeInt(this->GetDimensions(dimension), header);
    header += 4;
  }
  for (unsigned int dimension = 0; dimension < 3; ++dimension)
  {
    // physdim
    OMEZarrNGFFImageIO::EncodeInt(this->GetSpacing(dimension) * this->GetDimensions(dimension) * 1e3, header);
    header += 4;
  }
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_SliceThickness * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_SliceIncrement * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_StartPosition * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_DataRange[0]), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_DataRange[1]), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_MuScaling), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt(this->m_NumberOfSamples, header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt(this->m_NumberOfProjections, header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_ScanDistance * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_ScannerType), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_SampleTime * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_MeasurementIndex), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_Site), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_ReferenceLine * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_ReconstructionAlg), header);
  header += 4;
  OMEZarrNGFFImageIO::PadString(header, this->m_PatientName, 40);
  header += 40;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_Energy * 1e3), header);
  header += 4;
  OMEZarrNGFFImageIO::EncodeInt((int)(this->m_Intensity * 1e3), header);
  header += 4;
  const std::size_t fillSize = 83 * 4;
  std::memset(header, 0x00, fillSize);
  header += fillSize;
  // dataOffset
  const int dataOffset = 0;
  OMEZarrNGFFImageIO::EncodeInt(dataOffset, header);
  header += 4;

  this->m_HeaderSize = static_cast<SizeValueType>(dataOffset + 1) * 512;
  this->m_Compression = 0;

  file->write(this->m_RawHeader, 512);
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
