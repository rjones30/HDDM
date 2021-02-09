cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(HDDM VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ROOT support
if(NOT DEFINED ROOT_SUPPORT)
    set(ROOT_SUPPORT OFF CACHE BOOL "include support for ROOT data files")
endif()

# default ROOTSYS
if(NOT DEFINED ROOTSYS)
    set(ROOTSYS /usr CACHE PATH "root directory of local ROOT installation")
endif()

# HDF5 support
if(NOT DEFINED HDF5_SUPPORT)
    set(HDF5_SUPPORT OFF CACHE BOOL "include support for hdf5 data files")
endif()

# default HDF5_ROOT
if(NOT DEFINED HDF5_ROOT)
    set(HDF5_ROOT /usr CACHE PATH "root directory of local hdf5 installation")
endif()

# default XERCSCROOT
if(NOT DEFINED XERCESCROOT)
    set(XERCESCROOT /usr CACHE PATH "root directory of local xerces-c installation")
endif()

# add directories to the include list
list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/xstream/include)
include_directories(PUBLIC ${EXTRA_INCLUDES})

# add the xstream library
add_subdirectory(xstream)

# add the libraries to the link list
list(APPEND EXTRA_LIBS xstream)

# build the executables
add_subdirectory(src)

# install the scripts
add_subdirectory(scripts)
