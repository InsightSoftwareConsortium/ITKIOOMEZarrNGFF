itk_module_test()

set(IOZ5Tests
  itkMinimalStandardRandomVariateGeneratorTest.cxx
  itkMyFilterTest.cxx
  )

CreateTestDriver(IOZ5 "${IOZ5-Test_LIBRARIES}" "${IOZ5Tests}")

itk_add_test(NAME itkMinimalStandardRandomVariateGeneratorTest
  COMMAND IOZ5TestDriver itkMinimalStandardRandomVariateGeneratorTest
  )

itk_add_test(NAME itkMyFilterTest
  COMMAND IOZ5TestDriver
    --compare
    ${ITK_TEST_OUTPUT_DIR}/itkMyFilterTestOutput.mha
    DATA{Baseline/itkMyFilterTestOutput.mha}
  itkMyFilterTest
    ${ITK_TEST_OUTPUT_DIR}/itkMyFilterTestOutput.mha
  )
