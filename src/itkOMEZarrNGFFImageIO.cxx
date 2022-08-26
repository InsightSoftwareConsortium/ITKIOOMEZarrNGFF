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

#include "itkOMEZarrNGFFImageIO.h"
#include "itkIOCommon.h"
#include "itkIntTypes.h"

#include "netcdf.h"

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

  this->Self::SetCompressor("");
  this->Self::SetMaximumCompressionLevel(9);
  this->Self::SetCompressionLevel(2);
}


void
OMEZarrNGFFImageIO::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent); // TODO: either add stuff here, or remove this override
}

IOComponentEnum
netCDFToITKComponentType(const nc_type netCDFComponentType)
{
  switch (netCDFComponentType)
  {
    case NC_BYTE:
      return IOComponentEnum::CHAR;

    case NC_UBYTE:
      return IOComponentEnum::UCHAR;

    case NC_SHORT:
      return IOComponentEnum::SHORT;

    case NC_USHORT:
      return IOComponentEnum::USHORT;

    case NC_INT:
      return IOComponentEnum::INT;

    case NC_UINT:
      return IOComponentEnum::UINT;

    case NC_INT64:
      return IOComponentEnum::LONGLONG;

    case NC_UINT64:
      return IOComponentEnum::ULONGLONG;

    case NC_FLOAT:
      return IOComponentEnum::FLOAT;

    case NC_DOUBLE:
      return IOComponentEnum::DOUBLE;

    default:
      return IOComponentEnum::UNKNOWNCOMPONENTTYPE;
  }
}

nc_type
itkToNetCDFComponentType(const IOComponentEnum itkComponentType)
{
  switch (itkComponentType)
  {
    case IOComponentEnum::UNKNOWNCOMPONENTTYPE:
      return NC_NAT;

    case IOComponentEnum::CHAR:
      return NC_BYTE;

    case IOComponentEnum::UCHAR:
      return NC_UBYTE;

    case IOComponentEnum::SHORT:
      return NC_SHORT;

    case IOComponentEnum::USHORT:
      return NC_USHORT;

    // "long" is a silly type because it basically guaranteed not to be
    // cross-platform across 32-vs-64 bit machines, but we can figure out
    // a cross-platform way of storing the information.
    case IOComponentEnum::LONG:
      return (4 == sizeof(long)) ? NC_INT : NC_INT64;

    case IOComponentEnum::ULONG:
      return (4 == sizeof(long)) ? NC_UINT : NC_UINT64;

    case IOComponentEnum::INT:
      return NC_INT;

    case IOComponentEnum::UINT:
      return NC_UINT;

    case IOComponentEnum::LONGLONG:
      return NC_INT64;

    case IOComponentEnum::ULONGLONG:
      return NC_UINT64;

    case IOComponentEnum::FLOAT:
      return NC_FLOAT;

    case IOComponentEnum::DOUBLE:
      return NC_DOUBLE;

    case IOComponentEnum::LDOUBLE:
      return NC_NAT; // Long double not supported by netCDF

    default:
      return NC_NAT;
  }
}


bool
OMEZarrNGFFImageIO::CanReadFile(const char * filename)
{
  return this->HasSupportedWriteExtension(filename, true);
  // if (!this->HasSupportedWriteExtension(filename, true))
  //{
  //  return false;
  //}

  try
  {
    int result = nc_open(getNCFilename(filename), NC_NOWRITE, &m_NCID);
    if (this->GetDebug())
    {
      std::cout << "netCDF error: " << nc_strerror(result) << std::endl;
    }

    if (!result) // if it was opened correctly, we should close it
    {
      int nVars;
      result = nc_inq(m_NCID, nullptr, &nVars, nullptr, nullptr);

      nc_close(m_NCID);
      return !result && (nVars > 0);
    }
    return false;
  }
  catch (...) // file cannot be opened, access denied etc.
  {
    return false;
  }
}

// Make netCDF call, and error check it.
// Requires variable int r; to be defined.
#define netCDF_call(call)                                  \
  r = call;                                                \
  if (r) /* error */                                       \
  {                                                        \
    nc_close(m_NCID); /* clean up a little */              \
    itkExceptionMacro("netCDF error: " << nc_strerror(r)); \
  }

void
OMEZarrNGFFImageIO::ReadImageInformation()
{
  if (this->m_FileName.empty())
  {
    itkExceptionMacro("FileName has not been set.");
  }

  int r;
  netCDF_call(nc_open(getNCFilename(this->m_FileName), NC_NOWRITE, &m_NCID));

  int nDims;
  int nVars;
  int nAttr; // we will ignore attributes for now
  netCDF_call(nc_inq(m_NCID, &nDims, &nVars, &nAttr, nullptr));

  if (nVars == 0)
  {
    itkWarningMacro("The file '" << this->m_FileName << "' contains no arrays.");
    return;
  }

  char    name[NC_MAX_NAME + 1];
  nc_type ncType;
  int     dimIDs[NC_MAX_VAR_DIMS];
  netCDF_call(nc_inq_var(m_NCID, 0, name, &ncType, &nDims, dimIDs, nullptr));

  if (nVars > 1)
  {
    itkWarningMacro("The file '" << this->m_FileName << "' contains more than one array. The first one (" << name
                                 << ") will be read. The others will be ignored. ");
  }

  // TODO: examine all the variables, and prefer the one named 'image' or '0'

  int    cDim = 0;
  size_t len;
  netCDF_call(nc_inq_dim(m_NCID, dimIDs[nDims - 1], name, &len));
  if (std::string(name) == "c") // last dimension is component
  {
    cDim = 1;
    this->SetNumberOfComponents(len);
    this->SetPixelType(CommonEnums::IOPixel::VECTOR); // TODO: RGB/A, Tensor etc.
  }

  this->SetNumberOfDimensions(nDims - cDim);
  for (unsigned d = 0; d < nDims - cDim; ++d)
  {
    netCDF_call(nc_inq_dim(m_NCID, dimIDs[d], name, &len));
    this->SetDimensions(nDims - cDim - d - 1, len);
  }

  this->SetComponentType(netCDFToITKComponentType(ncType));
}


void
OMEZarrNGFFImageIO::Read(void * buffer)
{
  int r;
  if (this->GetLargestRegion() != m_IORegion)
  {
    // Stream the data in chunks
  }
  else
  {
    int     varID = 0; // always zero for now
    nc_type netCDF_type = itkToNetCDFComponentType(this->GetComponentType());
    switch (netCDF_type)
    {
      case NC_BYTE:
        netCDF_call(nc_get_var_text(m_NCID, varID, static_cast<char *>(buffer)));
        break;
      case NC_UBYTE:
        netCDF_call(nc_get_var_ubyte(m_NCID, varID, static_cast<unsigned char *>(buffer)));
        break;
      case NC_SHORT:
        netCDF_call(nc_get_var_short(m_NCID, varID, static_cast<short *>(buffer)));
        break;
      case NC_USHORT:
        netCDF_call(nc_get_var_ushort(m_NCID, varID, static_cast<unsigned short *>(buffer)));
        break;
      case NC_INT:
        netCDF_call(nc_get_var_int(m_NCID, varID, static_cast<int *>(buffer)));
        break;
      case NC_UINT:
        netCDF_call(nc_get_var_uint(m_NCID, varID, static_cast<unsigned int *>(buffer)));
        break;
      case NC_INT64:
        netCDF_call(nc_get_var_longlong(m_NCID, varID, static_cast<long long *>(buffer)));
        break;
      case NC_UINT64:
        netCDF_call(nc_get_var_ulonglong(m_NCID, varID, static_cast<unsigned long long *>(buffer)));
        break;
      case NC_FLOAT:
        netCDF_call(nc_get_var_float(m_NCID, varID, static_cast<float *>(buffer)));
        break;
      case NC_DOUBLE:
        netCDF_call(nc_get_var_double(m_NCID, varID, static_cast<double *>(buffer)));
        break;
      default:
        itkExceptionMacro("Unsupported component type: " << this->GetComponentTypeAsString(this->m_ComponentType));
        break;
    }

    netCDF_call(nc_close(m_NCID));
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

  return this->HasSupportedWriteExtension(name, true);
}


void
OMEZarrNGFFImageIO::WriteImageInformation()
{
  if (this->m_FileName.empty())
  {
    itkExceptionMacro("FileName has not been set.");
  }

  // we will do everything in Write() method
}


void
OMEZarrNGFFImageIO::Write(const void * buffer)
{
  int r;
  netCDF_call(nc_create(getNCFilename(this->m_FileName), NC_CLOBBER, &m_NCID));

  const std::vector<std::string> dimensionNames = { "c", "i", "j", "k", "t" };

  unsigned nDims = this->GetNumberOfDimensions();

  // handle component dimension
  unsigned cDims = 0;
  if (this->GetNumberOfComponents() > 1)
  {
    cDims = 1;
  }

  int dimIDs[MaximumDimension];
  netCDF_call(nc_def_dim(m_NCID, dimensionNames[0].c_str(), this->GetNumberOfComponents(), &dimIDs[nDims]));

  // handle other dimensions
  for (unsigned d = 0; d < nDims; ++d)
  {
    unsigned dSize = this->GetDimensions(nDims - 1 - d);
    netCDF_call(nc_def_dim(m_NCID, dimensionNames[nDims - d].c_str(), dSize, &dimIDs[d]));
  }

  int netCDF_type = itkToNetCDFComponentType(this->m_ComponentType);
  int varID;
  netCDF_call(nc_def_var(m_NCID, "image", netCDF_type, nDims + cDims, dimIDs, &varID));
  netCDF_call(nc_enddef(m_NCID));

  switch (netCDF_type)
  {
    case NC_BYTE:
      netCDF_call(nc_put_var_text(m_NCID, varID, static_cast<const char *>(buffer)));
      break;
    case NC_UBYTE:
      netCDF_call(nc_put_var_ubyte(m_NCID, varID, static_cast<const unsigned char *>(buffer)));
      break;
    case NC_SHORT:
      netCDF_call(nc_put_var_short(m_NCID, varID, static_cast<const short *>(buffer)));
      break;
    case NC_USHORT:
      netCDF_call(nc_put_var_ushort(m_NCID, varID, static_cast<const unsigned short *>(buffer)));
      break;
    case NC_INT:
      netCDF_call(nc_put_var_int(m_NCID, varID, static_cast<const int *>(buffer)));
      break;
    case NC_UINT:
      netCDF_call(nc_put_var_uint(m_NCID, varID, static_cast<const unsigned int *>(buffer)));
      break;
    case NC_INT64:
      netCDF_call(nc_put_var_longlong(m_NCID, varID, static_cast<const long long *>(buffer)));
      break;
    case NC_UINT64:
      netCDF_call(nc_put_var_ulonglong(m_NCID, varID, static_cast<const unsigned long long *>(buffer)));
      break;
    case NC_FLOAT:
      netCDF_call(nc_put_var_float(m_NCID, varID, static_cast<const float *>(buffer)));
      break;
    case NC_DOUBLE:
      netCDF_call(nc_put_var_double(m_NCID, varID, static_cast<const double *>(buffer)));
      break;
    default:
      itkExceptionMacro("Unsupported component type: " << this->GetComponentTypeAsString(this->m_ComponentType));
      break;
  }

  netCDF_call(nc_close(m_NCID));
}

} // end namespace itk
