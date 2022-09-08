cmake_minimum_required(VERSION 3.16.3)
cmake_policy(SET CMP0135 NEW)
# cmake_policy(SET CMP0002 OLD) # doesn't help

project(IOOMEZarrNGFF)

set(IOOMEZarrNGFF_LIBRARIES IOOMEZarrNGFF)

if(ITK_SOURCE_DIR)
  message(FATAL_ERROR "${PROJECT_NAME} currently does not support being built as part of ITK_DIR
  It needs to be built of out source, and pointed to a certain ITK build.")
endif()

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        unset(MATCHED)
        string(REGEX MATCH "CMAKE_" MATCHED ${_variableName})
        if (MATCHED) # skip all of CMake's variables - there is a lot of them
            continue()
        endif()

        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

include(FetchContent)
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING TRUE CACHE BOOL "Should ${PROJECT_NAME} tests be built?")
endif()
set(_itk_build_testing ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "tests" FORCE)

set(_itk_build_shared ${BUILD_SHARED_LIBS})

find_package(ITK 5.0 REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# dump_cmake_variables("ZLIB")

# Now specify all the libraries we want to fetch and compile
set(ZLIB_LIBRARY ${ITKZLIB_LIBRARIES})
set(ZLIB_INCLUDE_DIR ${ITKZLIB_INCLUDE_DIRS})
add_library(ZLIB::ZLIB ALIAS ${ITKZLIB_LIBRARIES})
set(BUILD_CURL_EXE OFF)
FetchContent_Declare(
  CURL
  GIT_REPOSITORY https://github.com/curl/curl
  GIT_TAG        curl-7_84_0
)
FetchContent_MakeAvailable(CURL)
# dump_cmake_variables("CURL")

# set(BUILD_DOC OFF)
# set(BUILD_EXAMPLES OFF)
# set(BUILD_REGRESS OFF)
# set(BUILD_TOOLS OFF)
set(netcdf_ENABLE_ZSTD OFF)
set(ENABLE_ZSTD OFF)
# FetchContent_Declare(
#   libzip
#   GIT_REPOSITORY https://github.com/nih-at/libzip
#   GIT_TAG        v1.9.2
# )
# FetchContent_MakeAvailable(libzip)

# netCDF needs zlib and curl
set(ZLIB_INCLUDE_DIR ${ITKZLIB_INCLUDE_DIRS} CACHE PATH "Path to zlib include" FORCE)
set(ZLIB_LIBRARY ${ITKZLIB_LIBRARIES} CACHE FILEPATH "Path to zlib library" FORCE)

set(CURL_INCLUDE_DIR "${CURL_SOURCE_DIR}/include" CACHE PATH "Path to zlib include" FORCE)
set(CURL_LIBRARY_DEBUG ${LIBCURL_OUTPUT_NAME} CACHE FILEPATH "Path to zlib library" FORCE)
set(CURL_LIBRARY_RELEASE ${LIBCURL_OUTPUT_NAME} CACHE FILEPATH "Path to zlib library" FORCE)


# message("before FetchContent_Declare")
# dump_cmake_variables("ITK_")

get_filename_component(ITK_ROOT_DIR ${ITK_USE_FILE} DIRECTORY)
get_filename_component(ITK_ROOT_DIR "${ITK_ROOT_DIR}/../" ABSOLUTE)
# message("ITK_ROOT_DIR: ${ITK_ROOT_DIR}")
set(H5_src "${ITK_ROOT_DIR}/Modules/ThirdParty/HDF5/src/itkhdf5/src")
set(H5_build "${ITK_DIR}/Modules/ThirdParty/HDF5/src/itkhdf5/src")
set(H5_hl "${ITK_ROOT_DIR}/Modules/ThirdParty/HDF5/src/itkhdf5/hl/src")


# satisfy netCDF's need for HDF5 definitions
set(HDF5_INCLUDE_DIR "${H5_build};${H5_src};${H5_hl}")
# message("HDF5_INCLUDE_DIR: ${HDF5_INCLUDE_DIR}")
set(HDF5_VERSION "1.12.1")
# set(HDF5_C_LIBRARY ${ITKHDF5_LIBRARIES})
# set(HDF5_HL_LIBRARY ${ITKHDF5_LIBRARIES})
set(HDF5_LIBRARIES ${ITKHDF5_LIBRARIES})

# message("after find HDF5")
# dump_cmake_variables("HDF5")

set(BUILD_TESTSETS OFF CACHE BOOL "netCDF tests" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "netCDF tests" FORCE)
set(BUILD_UTILITIES OFF CACHE BOOL "netCDF tools" FORCE)
set(ENABLE_DAP2 ON CACHE BOOL "Enable LDAP2 in netCDF" FORCE)
FetchContent_Declare(
  netCDF
  GIT_REPOSITORY https://github.com/dzenanz/netcdf-c # we need a fork, otherwise it is to hard to provide HDF5 to netCDF
  GIT_TAG        itkSpecific # main # v4.9.0
)

# message("HDF5 after FetchContent_Declare")
# dump_cmake_variables("HDF5")

FetchContent_MakeAvailable(netCDF)

# message("after FetchContent_MakeAvailable")
# dump_cmake_variables()

# Restore build options
set(BUILD_TESTING ${_itk_build_testing} CACHE BOOL "tests" FORCE)
set(BUILD_SHARED_LIBS ${_itk_build_shared} CACHE BOOL "DLLs" FORCE)

# find_package(netCDF)

function (_get_all_cmake_targets out_var current_dir)
    get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

    foreach(subdir ${subdirs})
        _get_all_cmake_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()

# Run at end of top-level CMakeLists
_get_all_cmake_targets(all_targets ${CMAKE_CURRENT_LIST_DIR})

list(APPEND IOOMEZarrNGFF_INCLUDE_DIRS ${netCDF_SOURCE_DIR}/include ${HDF5_INCLUDE_DIR} ${ITKHDF5_INCLUDE_DIRS})
# message("IOOMEZarrNGFF_INCLUDE_DIRS: ${IOOMEZarrNGFF_INCLUDE_DIRS}")
list(APPEND IOOMEZarrNGFF_LIBRARIES netCDF::netcdf ${HDF5_LIBRARIES})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# dump_cmake_variables("HDF5")
include(ITKModuleExternal)

# find_package(xtensor-zarr CONFIG)

# if(NOT ITK_SOURCE_DIR)
#   find_package(ITK 5.0 REQUIRED)
#   list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
#   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#   include(ITKModuleExternal)
# else()
#   set(ITK_DIR ${CMAKE_BINARY_DIR})
#   itk_module_impl()
# endif()

# dump_cmake_variables("IOOMEZarrNGFF")
