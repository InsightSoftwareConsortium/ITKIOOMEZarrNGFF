cmake_minimum_required(VERSION 3.24)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(IOOMEZarrNGFF CXX C ASM_NASM ASM)

set(IOOMEZarrNGFF_LIBRARIES IOOMEZarrNGFF)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK 5.0 REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  include(ITKModuleExternal)
else()
  set(ITK_DIR ${CMAKE_BINARY_DIR})
  itk_module_impl()
endif()
