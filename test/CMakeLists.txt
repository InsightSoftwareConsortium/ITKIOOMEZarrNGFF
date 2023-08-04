itk_module_test()

set(IOOMEZarrNGFFTests
  itkOMEZarrNGFFHTTPTest.cxx
  itkOMEZarrNGFFImageIOTest.cxx
  itkOMEZarrNGFFInMemoryTest.cxx
  itkOMEZarrNGFFReadTest.cxx
  itkOMEZarrNGFFReadSubregionTest.cxx
  )

CreateTestDriver(IOOMEZarrNGFF "${IOOMEZarrNGFF-Test_LIBRARIES}" "${IOOMEZarrNGFFTests}")

# itk_add_test(NAME IOOMEZarrNGFF_SmallSpectra_nrrd
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${CMAKE_CURRENT_BINARY_DIR}/SmallSpectra.nrrd
#     itkOMEZarrNGFFImageIOTest
#       DATA{Input/SmallSpectra.zip}
#       ${CMAKE_CURRENT_BINARY_DIR}/SmallSpectra.nrrd
#   )
# 
# itk_add_test(NAME IOOMEZarrNGFF_SmallSpectra_zip
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Input/SmallSpectra.zip}
#       ${CMAKE_CURRENT_BINARY_DIR}/SmallSpectra.zip
#     itkOMEZarrNGFFImageIOTest
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${CMAKE_CURRENT_BINARY_DIR}/SmallSpectra.zip
#   )

# 2D test with identity metadata
itk_add_test(NAME IOOMEZarrNGFF_cthead1_mha
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.zarr
  )

# 3D test image with non-identity spacing
itk_add_test(NAME IOOMEZarrNGFF_DzZ_T1seg
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${CMAKE_CURRENT_BINARY_DIR}/DzZ_T1seg.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${CMAKE_CURRENT_BINARY_DIR}/DzZ_T1seg.zarr
  )

# 3D test image with non-identity origin and spacing
itk_add_test(NAME IOOMEZarrNGFF_OriginSpacing
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/PV6.0_FLASH_FLOAT32.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/PV6.0_FLASH_FLOAT32.zarr
  )

itk_add_test(NAME IOOMEZarrNGFF_cthead1_zipWrite
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.zarr.zip
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.zarr.zip
  )

itk_add_test(NAME IOOMEZarrNGFF_cthead1_zipRead
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/cthead1.zarr.zip}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.mha
    itkOMEZarrNGFFImageIOTest
      DATA{Baseline/cthead1.zarr.zip}
      ${CMAKE_CURRENT_BINARY_DIR}/cthead1.mha
  )

itk_add_test(NAME IOOMEZarrNGFF_inMemory_zip
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/cthead1.zarr.zip}
      ${CMAKE_CURRENT_BINARY_DIR}/memory.zip
    itkOMEZarrNGFFInMemoryTest
      DATA{Baseline/cthead1.zarr.zip}
      ${CMAKE_CURRENT_BINARY_DIR}/memory.zip
  )
  
# 2D test with identity metadata
itk_add_test(NAME IOOMEZarrNGFF_readSubregion
COMMAND IOOMEZarrNGFFTestDriver
  --compare
    DATA{Baseline/cthead1Subregion.mha}
    ${CMAKE_CURRENT_BINARY_DIR}/cthead1Subregion.mha
  itkOMEZarrNGFFReadSubregionTest
    DATA{Input/cthead1.mha}
    ${CMAKE_CURRENT_BINARY_DIR}/cthead1.zarr
    ${CMAKE_CURRENT_BINARY_DIR}/cthead1Subregion.mha
)

# HTTP test with encoded test cases
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_2D
  COMMAND IOOMEZarrNGFFTestDriver
    itkOMEZarrNGFFHTTPTest
    0
)
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_3D
  COMMAND IOOMEZarrNGFFTestDriver
    itkOMEZarrNGFFHTTPTest
    1
)
