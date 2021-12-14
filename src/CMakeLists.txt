set(IOOMEZarrNGFF_SRCS
  )

itk_module_add_library(IOOMEZarrNGFF ${IOOMEZarrNGFF_SRCS})
target_link_libraries(IOOMEZarrNGFF PUBLIC xtensor-zarr)
