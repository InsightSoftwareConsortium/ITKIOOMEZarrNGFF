itk_module_test()

set(IOOMEZarrNGFFTests
  itkOMEZarrNGFFImageIOTest.cxx
  )

CreateTestDriver(IOOMEZarrNGFF "${IOOMEZarrNGFF-Test_LIBRARIES}" "${IOOMEZarrNGFFTests}")

# itk_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTest
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.nrrd
#     itkOMEZarrNGFFImageIOTest
#       DATA{Input/SmallSpectra.zip}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.nrrd
#   )
# 
# itk_add_test(NAME itkOMEZarrNGFFImageIOWriteConvertTest
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Input/SmallSpectra.zip}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.zip
#     itkOMEZarrNGFFImageIOTest
#       DATA{Baseline/SmallSpectra.nrrd}
#       ${ITK_TEST_OUTPUT_DIR}/SmallSpectra.zip
#   )
# set_tests_properties(itkOMEZarrNGFFImageIOWriteConvertTest PROPERTIES DEPENDS "itkOMEZarrNGFFImageIOReadConvertTest")

# 2D test with identity metadata
itk_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTestCT1
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zarr
  )

# 3D test image with non-identity spacing
itk_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTestDzZ_T1seg
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${ITK_TEST_OUTPUT_DIR}/DzZ_T1seg.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/DzZ_Seeds.seg.nrrd}
      ${ITK_TEST_OUTPUT_DIR}/DzZ_T1seg.zarr
  )

# 3D test image with non-identity origin and spacing
itk_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTest_OriginSpacing
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${ITK_TEST_OUTPUT_DIR}/PV6.0_FLASH_FLOAT32.zarr
    itkOMEZarrNGFFImageIOTest
      DATA{Input/PV6.0_FLASH_FLOAT32.mha}
      ${ITK_TEST_OUTPUT_DIR}/PV6.0_FLASH_FLOAT32.zarr
  )


# itk_add_test(NAME itkOMEZarrNGFFImageIOWriteConvertTestCT1
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Baseline/cthead1.zip}
#       ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
#     itkOMEZarrNGFFImageIOTest
#       DATA{Baseline/cthead1.zip}
#       ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
#   )
# set_tests_properties(itkOMEZarrNGFFImageIOWriteConvertTestCT1 PROPERTIES DEPENDS "itkOMEZarrNGFFImageIOReadConvertTestCT1")
