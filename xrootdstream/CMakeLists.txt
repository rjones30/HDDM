# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions( -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 )

# build against the xrootd client library
#find_package(xrootd REQUIRED)

add_library(xrootdstream STATIC xrootdstream.cpp)
set_property(TARGET xrootdstream PROPERTY POSITION_INDEPENDENT_CODE ON)
if(XROOTD_FOUND)
    target_include_directories(xrootdstream PRIVATE ${XROOTD_INCLUDE_DIRS})
    target_link_libraries(xrootdstream PRIVATE ${XROOTD_LINK_LIBRARIES})
else()
    target_include_directories(xrootdstream PRIVATE /usr/include/xrootd ${CMAKE_INSTALL_PREFIX}/include/xrootd)
    target_link_directories(xrootdstream PRIVATE ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64)
endif()
target_link_libraries(xrootdstream PRIVATE xrootd)

install(TARGETS xrootdstream DESTINATION lib)
install(FILES xrootdstream.hpp DESTINATION include)
