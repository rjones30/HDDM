cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0074 NEW)

# set the project name and version
project(HDDM VERSION 1.0)

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${HDDM_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${HDDM_VERSION_MINOR}")
include(CPack)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# specify defaults for optional components
option(ENABLE_ISTREAM_OVER_HTTP "Enable hddm_X::istream over HTTP" ON)
option(ENABLE_ISTREAM_OVER_XROOTD "Enable hddm_X::istream over XRootD" ON)

# look for installed packages in local build area
list(APPEND CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/cmake)

# check for presence of compression libraries
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)

# Xerces-C support
if(DEFINED XERCESCROOT)
    message("Looking in ${XERCESCROOT} for local Xerces-C installation")
    find_package(XercesC PATHS ${XERCESCROOT} NO_DEFAULT_PATH REQUIRED)
endif()
find_package(XercesC REQUIRED)

# ROOT support
if(DEFINED ROOTSYS)
    message("Looking in ${ROOTSYS} for local ROOT installation")
    find_package(ROOT PATHS ${ROOTSYS} REQUIRED NO_DEFAULT_PATH)
endif()
find_package(ROOT QUIET)

# HDF5 support
if(DEFINED HDF5_ROOT)
    find_package(HDF5 REQUIRED COMPONENTS C CXX HL PATHS ${HDF5_ROOT} NO_DEFAULT_PATH)
elseif(EXISTS "${CMAKE_INSTALL_PREFIX}/share/cmake/hdf5-config.cmake")
    message("Looking in ${CMAKE_INSTALL_PREFIX} for local HDF5 installation")
    set(HDF5_DIR "${CMAKE_INSTALL_PREFIX}/share/cmake")
    find_package(HDF5 REQUIRED COMPONENTS C CXX HL PATHS ${HDF5_DIR} NO_DEFAULT_PATH)
    if(NOT DEFINED HDF5_INCLUDE_DIRS)
        set(HDF5_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
    endif()
    set(CMAKE_LIBRARY_PATH "${CMAKE_INSTALL_PREFIX}/lib64" "${CMAKE_INSTALL_PATH}/lib")
    find_library(HDF5_LIBRARIES hdf5)
    find_library(HDF5_HL_LIBRARIES hdf5_hl)
else()
    find_package(HDF5 REQUIRED COMPONENTS C CXX HL)
endif()

# XROOTD support
if(DEFINED XROOTD_ROOT)
    find_package(XRootD PATHS ${XROOTD_ROOT} NO_DEFAULT_PATH)
endif()

# xalan-j2 package
if(NOT DEFINED XALANJ2_PATH)
    set(XALANJ2_PATH /usr/share/java)
endif()
if(EXISTS ${XALANJ2_PATH}/xalan2.jar)
    message("-- Found xalan2 at ${XALANJ2_PATH}/xalan2.jar")
elseif(EXISTS ${XALANJ2_PATH}/xalan-j2.jar AND EXISTS ${XALANJ2_PATH}/xalan-j2-serializer.jar)
    message("-- Found xalan-j2 at ${XALANJ2_PATH}/xalan-j2.jar")
else()
    message("Cannot find xalan-j2 at ${XALANJ2_PATH}/xalan-j2.jar, some schema utilities will fail!")
endif()
   
# xmllint command
if(NOT DEFINED XMLLINT_PATH)
    set(XMLLINT_PATH /usr/bin)
endif()
if(EXISTS ${XMLLINT_PATH}/xmllint)
    message("-- Found xmllint at ${XMLLINT_PATH}/xmllint")
else()
    message("Cannot find xmllint at ${XMLLINT_PATH}/xmllint, some schema utilities will fail!")
endif()

# libtirpc
include_directories(${CMAKE_SOURCE_DIR}/src/tirpc)
install(FILES ${CMAKE_SOURCE_DIR}/src/unistd_win32.h DESTINATION include)

# add the xstream library
add_subdirectory(xstream)

# add the streaming library components
if(ENABLE_ISTREAM_OVER_HTTP)
    add_definitions(-DENABLE_ISTREAM_OVER_HTTP=1)
    add_subdirectory(httpstream)
endif()
if(ENABLE_ISTREAM_OVER_XROOTD)
    if(WIN32)
        message(FATAL_ERROR "The XRootD library does not currently build without errors.\n"
		"You can build HDDM on windows but you must disable http[s] streaming.")
    endif()
    add_definitions(-DENABLE_ISTREAM_OVER_XROOTD=1)
    add_subdirectory(xrootdstream)
endif()

# build the executables
add_subdirectory(src)

# install the scripts
add_subdirectory(scripts)

# install the schemas
add_subdirectory(schemas)

# install the examples
add_subdirectory(models)
add_subdirectory(examples)

# define a separate target for the example python modules
add_custom_target(modules_install ALL
      COMMAND ${CMAKE_SOURCE_DIR}/scripts/python_module_installer.sh
              ${CMAKE_BINARY_DIR}/models 
              ${CMAKE_INSTALL_PREFIX}
      DEPENDS python_modules
)

# package config
set(INCLUDE_INSTALL_DIR include/)
set(LIB_INSTALL_DIR lib/)
set(SYSCONFIG_INSTALL_DIR etc/hddm/)
#...
include(CMakePackageConfigHelpers)
configure_package_config_file(HddmConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/HddmConfig.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/hddm/cmake
  PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/HddmConfigVersion.cmake
  VERSION 1.0.0
  COMPATIBILITY SameMajorVersion )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/HddmConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/HddmConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/hddm/cmake )
