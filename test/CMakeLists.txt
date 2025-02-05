cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(HDDM_test VERSION 1.0)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
include(CPack)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build against the cpr library
find_package(cpr REQUIRED)

add_executable(httpstream_test httpstream_test.cpp)
target_include_directories(httpstream_test PRIVATE
                           ${CMAKE_SOURCE_DIR}/../src
                           ${CMAKE_INSTALL_PREFIX}/include
                          )
target_link_directories(httpstream_test PRIVATE
                        ${CMAKE_INSTALL_PREFIX}/lib
                        ${CMAKE_INSTALL_PREFIX}/lib64
                       )
target_link_libraries(httpstream_test PRIVATE cpr curl ssl crypto z)
install(TARGETS httpstream_test)

if(NOT WIN32)
    add_executable(xrootdstream_test xrootdstream_test.cpp)
    target_include_directories(xrootdstream_test PRIVATE
                               ${CMAKE_SOURCE_DIR}/../src
                               ${CMAKE_INSTALL_PREFIX}/include
                              )
    target_link_directories(xrootdstream_test PRIVATE
                            ${CMAKE_INSTALL_PREFIX}/lib
                            ${CMAKE_INSTALL_PREFIX}/lib64
                           )
    target_link_libraries(xrootdstream_test PRIVATE xrootd)
    install(TARGETS xrootdstream_test)
endif()
