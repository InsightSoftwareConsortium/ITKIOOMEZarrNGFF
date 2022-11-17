set(test_input_dir ${${itk-module}_SOURCE_DIR}/test/Input)
set(test_baseline_dir ${${itk-module}_SOURCE_DIR}/test/Baseline)

itk_python_expression_add_test(NAME itkOMEZarrNGFFImageIOInstantiateTestPython
  EXPRESSION "io = itk.OMEZarrNGFFImageIO.New()"
  )

itk_python_add_test(NAME itkOMEZarrNGFFImageIOReadConvertTestPython
  TEST_DRIVER_ARGS
  --compare
    DATA{${test_input_dir}/cthead1.png}
    ${ITK_TEST_OUTPUT_DIR}/cthead1.png
  COMMAND itkOMEZarrNGFFImageIOTestPython.py
    DATA{${test_input_dir}/cthead1.png}
    ${ITK_TEST_OUTPUT_DIR}/cthead1py.zarr
    ${ITK_TEST_OUTPUT_DIR}/cthead1.png
  )
