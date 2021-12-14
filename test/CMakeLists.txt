itk_module_test()

set(IOOMEZarrNGFFTests
  )

CreateTestDriver(IOOMEZarrNGFF "${IOOMEZarrNGFF-Test_LIBRARIES}" "${IOOMEZarrNGFFTests}")

#itk_add_test(NAME itkMinimalStandardRandomVariateGeneratorTest
#  COMMAND IOOMEZarrNGFFTestDriver itkMinimalStandardRandomVariateGeneratorTest
#  )
#
#itk_add_test(NAME itkMyFilterTest
#  COMMAND IOOMEZarrNGFFTestDriver
#    --compare
#    DATA{Baseline/itkMyFilterTestOutput.mha}
#    ${ITK_TEST_OUTPUT_DIR}/itkMyFilterTestOutput.mha
#  itkMyFilterTest
#    ${ITK_TEST_OUTPUT_DIR}/itkMyFilterTestOutput.mha
#  )
