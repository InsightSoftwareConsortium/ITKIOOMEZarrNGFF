set(IOOMEZarrNGFF_SRCS
  itkOMEZarrNGFFImageIO.cxx
  itkOMEZarrNGFFImageIOFactory.cxx
  )



# We use tensorstore backend
set(TENSORSTORE_USE_SYSTEM_ZLIB ON) # Provide ITK's bundled zlib
set(ZLIB_INCLUDE_DIR ${ITKZLIB_INCLUDE_DIRS} CACHE PATH "Path to zlib include" FORCE)
set(ZLIB_LIBRARY ${ITKZLIB_LIBRARIES} CACHE FILEPATH "Path to zlib library" FORCE)
add_library(ZLIB::ZLIB ALIAS ${ITKZLIB_LIBRARIES})

set(_itk_build_shared ${BUILD_SHARED_LIBS})
# we want to compile tensorstore as a static library
# that way, all of it can be included in IOOMEZarrNGFF.dll
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries?" FORCE)
set(CMAKE_FOLDER TensorStore)
include(FetchContent)
FetchContent_Declare(
  tensorstore
  GIT_REPOSITORY https://github.com/google/tensorstore
  GIT_TAG        1a02a91f56e4efd8dd9f04e83cda33da258cd106
)
FetchContent_MakeAvailable(tensorstore)
unset(CMAKE_FOLDER)
set(BUILD_SHARED_LIBS ${_itk_build_shared} CACHE BOOL "Build shared libraries?" FORCE) # restore original flag
unset(_itk_build_shared)

itk_module_add_library(IOOMEZarrNGFF ${IOOMEZarrNGFF_SRCS})

target_link_libraries(IOOMEZarrNGFF PRIVATE tensorstore::tensorstore tensorstore::all_drivers)
