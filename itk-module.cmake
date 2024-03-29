# the top-level README is used for describing this module, just
# re-used it for documentation here
get_filename_component(MY_CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(READ "${MY_CURRENT_DIR}/README.md" DOCUMENTATION)

# itk_module() defines the module dependencies in IOOMEZarrNGFF
# By convention those modules outside of ITK are not prefixed with
# ITK.

# define the dependencies of the include module and the tests
itk_module(IOOMEZarrNGFF
  DEPENDS
    ITKIOImageBase
    ITKIOHDF5
    ITKZLIB
  TEST_DEPENDS
    ITKTestKernel
    ITKMetaIO
  FACTORY_NAMES
    ImageIO::OMEZarrNGFF
  DESCRIPTION
    "IO for images stored in zarr-backed OME-NGFF"
  EXCLUDE_FROM_DEFAULT
  ENABLE_SHARED
)
