cmake_minimum_required(VERSION 3.16.3)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

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

function (_get_all_cmake_targets out_var current_dir)
    get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

    foreach(subdir ${subdirs})
        _get_all_cmake_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()

# Get all propreties that cmake supports
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif()

function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_target_properties target)
    if(NOT TARGET ${target})
      message(STATUS "There is no target named '${target}'")
      return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
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
set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_GLOBAL TRUE) # from https://stackoverflow.com/a/48390363/276168
set(BUILD_CURL_EXE OFF)
FetchContent_Declare(
  CURL
  GIT_REPOSITORY https://github.com/curl/curl
  GIT_TAG        curl-7_84_0
)
FetchContent_MakeAvailable(CURL)

# dump_cmake_variables("CURL")
_get_all_cmake_targets(all_targets ${CMAKE_CURRENT_LIST_DIR})
# message("Targets: ${all_targets}")
# print_target_properties(libcurl)

# set(BUILD_DOC OFF)
# set(BUILD_EXAMPLES OFF)
# set(BUILD_REGRESS OFF)
# set(BUILD_TOOLS OFF)
set(ENABLE_ZSTD OFF CACHE BOOL "netCDF zstd")
# FetchContent_Declare(
#   libzip
#   GIT_REPOSITORY https://github.com/nih-at/libzip
#   GIT_TAG        v1.9.2
# )
# FetchContent_MakeAvailable(libzip)

# netCDF needs zlib and curl
set(ZLIB_INCLUDE_DIR ${ITKZLIB_INCLUDE_DIRS} CACHE PATH "Path to zlib include" FORCE)
set(ZLIB_LIBRARY ${ITKZLIB_LIBRARIES} CACHE FILEPATH "Path to zlib library" FORCE)

# curl is unusable from build directory via CURLConfig.cmake
# set(CURL_DIR "${FETCHCONTENT_BASE_DIR}/curl-build/generated" CACHE PATH "Path to CURLConfig.cmake" FORCE)
set(CURL_INCLUDE_DIR "${CURL_SOURCE_DIR}/include" CACHE PATH "Path to curl include" FORCE)
# get_cmake_property(curl_DEBUG libcurl LIBRARY_OUTPUT_NAME_DEBUG)
# message("curl_DEBUG: ${curl_DEBUG}")
# get_cmake_property(CURL_BINARY_DIR libcurl BINARY_DIR)

get_cmake_property(curl_RELEASE libcurl LIBRARY_OUTPUT_NAME_RELEASE)
link_directories("${CURL_BINARY_DIR}/lib")
set(CURL_LIBRARY_DEBUG ${LIBCURL_OUTPUT_NAME} CACHE FILEPATH "Path to curl library" FORCE)
set(CURL_LIBRARY_RELEASE ${LIBCURL_OUTPUT_NAME} CACHE FILEPATH "Path to curl library" FORCE)

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
set(ENABLE_NCZARR_FILTERS OFF CACHE BOOL "Enable NCZARR_FILTERS in netCDF" FORCE)
set(ENABLE_FILTER_TESTING OFF CACHE BOOL "Enable filter tests in netCDF" FORCE)
set(ENABLE_PLUGINS OFF CACHE BOOL "Enable plugins in netCDF" FORCE) # ON breaks the Python build
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
