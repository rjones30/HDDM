# add external libraries
list(APPEND EXTRA_LIBS xerces-c bz2 z)

if(DEFINED XERCESCROOT)
    list(APPEND EXTRA_INCLUDES ${XERCESCROOT}/include)
    link_directories(${XERCESCROOT}/lib)
endif()

# add the executables
add_executable(hddm-xml hddm-xml.cpp XString.cpp XParsers.cpp md5.c)
add_executable(xml-hddm xml-hddm.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-cpp hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-c hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-py hddm-py.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddmcat hddmcat.cpp)
if(${ROOT_SUPPORT})
    add_executable(hddm-root hddm-root.cpp XString.cpp XParsers.cpp md5.c)
endif()

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
target_link_libraries(hddm-c PUBLIC
                      "${EXTRA_LIBS}"
                     )
target_link_libraries(hddm-py PUBLIC
                      "${EXTRA_LIBS}"
                     )
target_link_libraries(hddmcat PUBLIC
                      "${EXTRA_LIBS}"
                     )

target_include_directories(hddm-xml PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )
target_include_directories(xml-hddm PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )
target_include_directories(hddm-cpp PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )
target_include_directories(hddm-c PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )
target_include_directories(hddm-py PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )
target_include_directories(hddmcat PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${CMAKE_CURRENT_BINARY_DIR}"
                           "${CMAKE_CURRENT_SOURCE_DIR}"
                           "${EXTRA_INCLUDES}"
                           )

if(${ROOT_SUPPORT})
    target_include_directories(hddm-root PUBLIC
                               "${PROJECT_BINARY_DIR}"
                               "${CMAKE_CURRENT_BINARY_DIR}"
                               "${CMAKE_CURRENT_SOURCE_DIR}"
                               "${EXTRA_INCLUDES}"
                               "${ROOTSYS}/include"
                               )
    execute_process(COMMAND ${ROOTSYS}/bin/root-config --libs OUTPUT_VARIABLE ROOTLIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    target_link_libraries(hddm-root PUBLIC
                          "${EXTRA_LIBS}"
                          "${ROOTLIBS}"
                          "-Wl,-rpath -Wl,${ROOTSYS}/lib"
                         )
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

install(TARGETS hddm-xml DESTINATION bin)
install(TARGETS xml-hddm DESTINATION bin)
install(TARGETS hddm-cpp DESTINATION bin)
install(TARGETS hddm-c DESTINATION bin)
install(TARGETS hddm-py DESTINATION bin)
install(TARGETS hddmcat DESTINATION bin)
if(${ROOT_SUPPORT})
    install(TARGETS hddm-root DESTINATION bin)
endif()
