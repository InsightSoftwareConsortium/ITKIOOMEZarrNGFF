# ITKIOOMEZarrNGFF

![Build Status](https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/workflows/Build,%20test,%20package/badge.svg)

[![PyPI Version](https://img.shields.io/pypi/v/itk-ioomezarrngff.svg)](https://pypi.python.org/pypi/itk-ioomezarrngff)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://github.com/InsightSoftwareConsortium/ITKIOOMEZarrNGFF/blob/main/LICENSE)

## Overview

This is ITK remote module for IO of images stored in zarr-backed [OME-NGFF](https://ngff.openmicroscopy.org/0.4/) file format.

## Build/install
C++ uses `CMake` as the build system. Python packages can be installed from PyPI:
```sh
pip install itk-ioomezarrngff
```

## Usage
Usage from C++ should not require any special action. In special situations, you might need to invoke:
```C++
itk::OMEZarrNGFFImageIOFactory::RegisterOneFactory();
```

In Python, we need to explicitly specify the IO, otherwise DICOM IO will be invoked because it is the built-in default for directories. Example:
```python
import sys
import itk
imageio = itk.OMEZarrNGFFImageIO.New()
image = itk.imread(sys.argv[1], imageio=imageio)
itk.imwrite(image, sys.argv[2], imageio=imageio, compression=False)
```
