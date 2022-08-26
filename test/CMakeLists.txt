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

itk_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTest
  COMMAND IOOMEZarrNGFFTestDriver
    --compare
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zip
    itkOMEZarrNGFFImageIOTest
      DATA{Input/cthead1.mha}
      ${ITK_TEST_OUTPUT_DIR}/cthead1.zip
  )

# itk_add_test(NAME itkOMEZarrNGFFImageIOWriteConvertTest
#   COMMAND IOOMEZarrNGFFTestDriver
#     --compare
#       DATA{Baseline/cthead1.zip}
#       ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
#     itkOMEZarrNGFFImageIOTest
#       DATA{Baseline/cthead1.zip}
#       ${ITK_TEST_OUTPUT_DIR}/cthead1.mha
#   )
