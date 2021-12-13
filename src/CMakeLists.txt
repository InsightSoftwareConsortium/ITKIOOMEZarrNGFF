set(IOOMEZarrNGFF_SRCS
  itkMinimalStandardRandomVariateGenerator.cxx
  )

itk_module_add_library(IOOMEZarrNGFF ${IOOMEZarrNGFF_SRCS})
target_link_libraries(IOOMEZarrNGFF PUBLIC xtensor-zarr)
