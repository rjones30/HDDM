# add external libraries
list(APPEND EXTRA_LIBS xerces-c bz2 z)

# add the executables
add_executable(hddm-xml hddm-xml.cpp XString.cpp XParsers.cpp md5.c)
add_executable(xml-hddm xml-hddm.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-cpp hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-py hddm-py.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddmcat hddmcat.cpp)

configure_file(VersionConfig.hpp.in VersionConfig.hpp)

target_link_libraries(hddm-xml PUBLIC
                      "${EXTRA_LIBS}"
                     )
target_link_libraries(xml-hddm PUBLIC
                      "${EXTRA_LIBS}"
                     )
target_link_libraries(hddm-cpp PUBLIC
                      "${EXTRA_LIBS}"
                     )
target_link_libraries(hddm-py PUBLIC
                      "${EXTRA_LIBS}"
                     )

target_include_directories(hddm-xml PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           )
target_include_directories(xml-hddm PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           )
target_include_directories(hddm-cpp PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           )
target_include_directories(hddm-py PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           )
target_include_directories(hddmcat PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           )
