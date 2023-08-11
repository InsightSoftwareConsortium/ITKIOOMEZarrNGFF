itk_module_test()

set(IOOMEZarrNGFFTests
  itkOMEZarrNGFFHTTPTest.cxx
  itkOMEZarrNGFFImageIOTest.cxx
  itkOMEZarrNGFFInMemoryTest.cxx
  itkOMEZarrNGFFReadTest.cxx
  itkOMEZarrNGFFReadSliceTest.cxx
  itkOMEZarrNGFFReadSubregionTest.cxx
  )

CreateTestDriver(IOOMEZarrNGFF "${IOOMEZarrNGFF-Test_LIBRARIES}" "${IOOMEZarrNGFFTests}")

# itk_add_test(NAME IOOMEZarrNGFF_SmallSpectra_nrrd
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.nrrd
#     itkOMEZarrNGFFImageIOTest
#       DATA{Input/SmallSpectra.zip}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.nrrd
#   )
# 
# itk_add_test(NAME IOOMEZarrNGFF_SmallSpectra_zip
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Input/SmallSpectra.zip}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.zip
#     itkOMEZarrNGFFImageIOTest
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.zip
#   )

# 2D test with identity metadata
itk_add_test(NAME IOOMEZarrNGFF_cthead1_mha
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr
  )

# 3D test image with non-identity spacing
itk_add_test(NAME IOOMEZarrNGFF_DzZ_T1seg
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${ITK_TEST_OUTPUT_DIR}/DzZ_T1seg.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${ITK_TEST_OUTPUT_DIR}/DzZ_T1seg.zarr
  )

# 3D test image with non-identity origin and spacing
itk_add_test(NAME IOOMEZarrNGFF_OriginSpacing
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${ITK_TEST_OUTPUT_DIR}/PV6.0_FLASH_FLOAT32.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${ITK_TEST_OUTPUT_DIR}/PV6.0_FLASH_FLOAT32.zarr
  )

itk_add_test(NAME IOOMEZarrNGFF_cthead1_zipWrite
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr.zip
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr.zip
  )

itk_add_test(NAME IOOMEZarrNGFF_cthead1_zipRead
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/cthead1.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
    itkOMEZarrNGFFImageIOTest
      DATA{Baseline/cthead1.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
  )

itk_add_test(NAME IOOMEZarrNGFF_inMemory_zip
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/cthead1.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/memory.zip
    itkOMEZarrNGFFInMemoryTest
      DATA{Baseline/cthead1.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/memory.zip
  )
  
# 2D test with identity metadata
itk_add_test(NAME IOOMEZarrNGFF_readSubregion
COMMAND IOOMEZarrNGFFTestDriver
  --compare
    DATA{Baseline/cthead1Subregion.mha}
    ${ITK_TEST_OUTPUT_DIR}/cthead1Subregion.mha
  itkOMEZarrNGFFReadSubregionTest
    DATA{Input/cthead1.mha}
    ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr
    ${ITK_TEST_OUTPUT_DIR}/cthead1Subregion.mha
)

itk_add_test(
  NAME IOOMEZarrNGFF_readTimeIndex0
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/simple_tyx_t0.mha}
      ${ITK_TEST_OUTPUT_DIR}/simple_tyx_t0.mha
    itkOMEZarrNGFFReadSliceTest
      DATA{Input/simple_tyx.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/simple_tyx_t0.mha
      2
      0
)

itk_add_test(
  NAME IOOMEZarrNGFF_readTimeIndex1
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/simple_tyx_t1.mha}
      ${ITK_TEST_OUTPUT_DIR}/simple_tyx_t0.mha
    itkOMEZarrNGFFReadSliceTest
      DATA{Input/simple_tyx.zarr.zip}
      ${ITK_TEST_OUTPUT_DIR}/simple_tyx_t0.mha
      2
      1
)

# HTTP test with encoded test cases
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_2D
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/yx_0.mha}
      ${ITK_TEST_OUTPUT_DIR}/yx_0.mha
    --compare
      DATA{Baseline/yx_1.mha}
      ${ITK_TEST_OUTPUT_DIR}/yx_1.mha
    itkOMEZarrNGFFHTTPTest
    0
    ${ITK_TEST_OUTPUT_DIR}/yx
)
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_3D
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/zyx_0.mha}
      ${ITK_TEST_OUTPUT_DIR}/zyx_0.mha
    --compare
      DATA{Baseline/zyx_1.mha}
      ${ITK_TEST_OUTPUT_DIR}/zyx_1.mha
    --compare
      DATA{Baseline/zyx_2.mha}
      ${ITK_TEST_OUTPUT_DIR}/zyx_2.mha
    itkOMEZarrNGFFHTTPTest
    1
    ${ITK_TEST_OUTPUT_DIR}/zyx
)
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_TimeSlice
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/slice_tyx_0.mha}
      ${ITK_TEST_OUTPUT_DIR}/slice_tyx_0.mha
    itkOMEZarrNGFFHTTPTest
    2
    ${ITK_TEST_OUTPUT_DIR}/slice_tyx
)
itk_add_test(
  NAME IOOMEZarrNGFFHTTP_TimeAndChannelSlice
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Baseline/slice_tczyx_2.mha}
      ${ITK_TEST_OUTPUT_DIR}/slice_tczyx_2.mha
    itkOMEZarrNGFFHTTPTest
    3
    ${ITK_TEST_OUTPUT_DIR}/slice_tczyx
)
