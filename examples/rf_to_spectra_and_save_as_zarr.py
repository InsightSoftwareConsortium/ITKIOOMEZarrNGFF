#!/usr/bin/env python3
import argparse
import pathlib
import itkConfig
import numpy as np
import xarray as xr
from numcodecs import Blosc

itkConfig.LazyLoading = True
import itk  # we require itk-ultrasound (in addition to regular ITK)


def GetFFTSpectra(rfImage, fft1DSize=32, fftStepSize=4):
    rfType = type(rfImage)
    SupportWindowType = itk.Image[itk.SS, rfImage.GetImageDimension()]
    sideLines = SupportWindowType.New()
    sideLines.CopyInformation(rfImage)
    sideLines.SetRegions(rfImage.GetLargestPossibleRegion())
    sideLines.Allocate()
    sideLines.FillBuffer(5)
    SpectraSupportWindowFilterType = itk.Spectra1DSupportWindowImageFilter[SupportWindowType]
    spectraSupportWindowFilter = SpectraSupportWindowFilterType.New()
    spectraSupportWindowFilter.SetInput(sideLines)
    spectraSupportWindowFilter.SetFFT1DSize(fft1DSize)
    spectraSupportWindowFilter.SetStep(fftStepSize)
    spectraSupportWindowFilter.UpdateLargestPossibleRegion()
    supportWindowImage = spectraSupportWindowFilter.GetOutput()
    supportWindowImage.GetLargestPossibleRegion()
    SpectraComponentType = itk.F
    SpectraImageType = itk.VectorImage[SpectraComponentType, rfImage.GetImageDimension()]
    SpectraFilterType = itk.Spectra1DImageFilter[rfType, type(supportWindowImage), SpectraImageType]
    spectraFilter = SpectraFilterType.New()
    spectraFilter.SetInput(rfImage)
    spectraFilter.SetSupportWindowImage(spectraSupportWindowFilter.GetOutput())
    spectraFilter.UpdateLargestPossibleRegion()
    spectra1DOutput = spectraFilter.GetOutput()
    return spectra1DOutput


def save_spectra(input_filename, output_filename):
    rf = itk.imread(input_filename, itk.F)
    print(f"{input_filename} has size {itk.size(rf)} and spacing {rf.GetSpacing()}")

    region = rf.GetLargestPossibleRegion()
    size = region.GetSize()
    if len(size) > 2:  # process the 3D image slice by slice
        z_size = size[2]
        size[2] = 0
        region.SetSize(size)

        for k in range(z_size):
            region = itk.ImageRegion[3]((0, 0, k), size)

            rf_slice = itk.extract_image_filter(rf, extraction_region=region,
                                                ttype=(type(rf), itk.Image[itk.F, 2]),
                                                direction_collapse_to_strategy=itk.ExtractImageFilterEnums.DirectionCollapseStrategy_DIRECTIONCOLLAPSETOIDENTITY)
            spectra_slice = GetFFTSpectra(rf_slice)

            spectra_slice_da = itk.xarray_from_image(spectra_slice)
            # spectra_slice_da = spectra_slice_da.isel(c=slice(5, 20))  # use only the high-power frequencies

            if k == 0:
                spectra_da = xr.concat([spectra_slice_da], 'z')  # initialize it
            else:
                spectra_da = xr.concat([spectra_da, spectra_slice_da], 'z')

            print(f" {k}", end="")

        print('')  # new line
        assert z_size == len(spectra_da['z'])
        z_coordinates = {'z': np.arange(0, z_size) * tuple(itk.spacing(rf))[-1] + rf.GetOrigin()[2]}
        spectra_da = spectra_da.assign_coords(z_coordinates)
        # increase dimension of direction from 2 to 3
        direction = spectra_da.attrs["direction"]
        direction = np.pad(direction, ((0, 1), (0, 1)), mode='constant', constant_values=0.0)
        direction[2, 2] = 1.0  # 3rd dimension along z (preserves identity)
        spectra_da.attrs["direction"] = direction
    else:  # process 2D case directly
        spectra_slice = GetFFTSpectra(rf)
        spectra_da = itk.xarray_from_image(spectra_slice)

    print(spectra_da)
    spectra_ds = spectra_da.to_dataset(name='spectra')
    compressor = Blosc(cname='zstd', clevel=3)  # compression level 3 is the default for zstd
    spectra_ds.to_zarr(output_filename + ".zip", encoding={"spectra": {"compressor": compressor}}, mode="w")  # overwrite if exists

    spectra = itk.image_from_xarray(spectra_da)
    itk.imwrite(spectra, output_filename + ".nrrd", True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', '-i', help='Path to ultrasound RF file', type=str)
    parser.add_argument('--output', '-o', help='Path to output spectra in zarr format', type=str)
    args = parser.parse_args()

    if args.input is not None and args.output is not None:
        save_spectra(args.input, args.output)
    else:
        print('Both arguments must be specified')
        print(parser.format_help())
