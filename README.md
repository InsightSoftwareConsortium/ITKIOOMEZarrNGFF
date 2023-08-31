# ITKIOOMEZarrNGFF

![Build Status](https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/workflows/Build,%20test,%20package/badge.svg)

[![PyPI Version](https://img.shields.io/pypi/v/itk-ioomezarrngff.svg)](https://pypi.python.org/pypi/itk-ioomezarrngff)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/blob/main/LICENSE)

## Overview

This is an [ITK](https://itk.org/) external module for IO of images stored in [Zarr](https://zarr.dev/)-backed
[OME-NGFF](https://ngff.openmicroscopy.org/0.4/) file format.

## Installation

The `itk-ioomezarrngff` Python package is available on the Python Package Index.

```sh
> python -m pip install itk-ioomezarrngff
```

## Example Usage

### C++

Usage from C++ should not require any special action. In special situations, you might need to invoke:

```C++
itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();
```

### Python

In Python, we need to explicitly specify the IO, otherwise DICOM IO will be invoked because it is the built-in default for directories. Example:
```python
import sys
import itk
imageio = itk.OMEZarrNGFFImageIO.New()
image = itk.imread(sys.argv[1], imageio=imageio)
itk.imwrite(image, sys.argv[2], imageio=imageio, compression=False)
```

## Build Instructions

ITKIOOMEZarrNGFF is an ITK C++ external module. It may be built with `CMake` and build tools such as
Ninja, gcc, or MSVC.

In the future ITKIOOMEZarrNGFF may be made available as an ITK remote module for direct
inclusion in the ITK build process.

### Prerequisites

- [CMake](https://cmake.org/)
- [Perl](https://www.perl.org/)
- [NASM](https://www.nasm.us/)
- An existing C++ build of [ITK](https://itk.org/)

### Building

ITKIOOMEZarrNGFF uses CMake for its build process.

```sh
# Create the build directory
> mkdir path/to/ITKIOOMEZarrNGFF-build
> cd path/to/ITKIOOMEZarrNGFF-build

# Configure the project
path/to/ITKIOOMEZarrNGFF-build > cmake -DITK_DIR:PATH="path/to/ITK-build" "path/to/ITKIOOMEZarrNGFF"

# Build the project
path/to/ITKIOOMEZarrNGFF-build > cmake --build . --config "Release"
```

### Testing

ITKIOOMEZarrNGFF tests may be run with [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html):

```sh
path/to/ITKIOOMEZarrNGFF-build > ctest -C "Release"
```

### Wrapping

See the [ITK Software Guide](https://itk.org/ItkSoftwareGuide.pdf) for information on wrapping ITK external modules for Python.

### Additional Notes

ITKIOOMEZarrNGFF depends on a fork of Google's [Tensorstore](https://github.com/google/tensorstore)
library for Zarr interoperation. The [InsightSoftwareConsortium/Tensorstore](https://github.com/InsightSoftwareConsortium/tensorstore)
fork implements additional functionality to maintain in-memory OME-Zarr stores.

----------------

## Acknowledgements

ITKIOOMEZarrNGFF was developed in part with support from:

- [NIH NIMH BRAIN Initiative](https://braininitiative.nih.gov/) under award 1RF1MH126732.
- The [Allen Institute for Neural Dynamics (AIND)](https://alleninstitute.org/division/neural-dynamics/).
