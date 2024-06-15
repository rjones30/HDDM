# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions( -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 )

# build against the cpr library
#find_package(cpr REQUIRED)

add_library(httpstream STATIC httpstream.cpp)
set_property(TARGET httpstream PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(httpstream PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
target_link_directories(httpstream PRIVATE ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64)
target_link_libraries(httpstream PRIVATE cpr curl ssl crypto z)

install(TARGETS httpstream DESTINATION lib)
install(FILES httpstream.hpp DESTINATION include)
