if(HDF5_FOUND)
    list(APPEND EXTRA_LIBRARIES ${HDF5_LIBRARIES})
    list(APPEND EXTRA_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})
    SET(HDF5_SUPPORT 1)
endif()

if(ROOT_FOUND)
    list(APPEND EXTRA_LIBRARIES ${ROOT_LIBRARIES})
    list(APPEND EXTRA_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS})
endif()

list(APPEND EXTRA_LIBRARIES xstream
                       ${XercesC_LIBRARIES}
                       ${BZIP2_LIBRARIES}
                       ${ZLIB_LIBRARIES}
    )

list(APPEND EXTRA_INCLUDE_DIRS ${PROJECT_BINARY_DIR}
                               ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${XercesC_INCLUDE_DIRS}
                               ${BZIP2_INCLUDE_DIRS}
                               ${ZLIB_INCLDUE_DIRS}
    )

# add the executables
add_executable(hddm-xml hddm-xml.cpp XString.cpp XParsers.cpp md5.c)
add_executable(xml-hddm xml-hddm.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-cpp hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-c hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-py hddm-py.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddmcat hddmcat.cpp)
if(${ROOT_FOUND})
    add_executable(hddm-root hddm-root.cpp XString.cpp XParsers.cpp md5.c)
endif()

configure_file(VersionConfig.hpp.in VersionConfig.hpp)

target_link_libraries(hddm-xml PUBLIC ${EXTRA_LIBRARIES})
target_link_libraries(xml-hddm PUBLIC ${EXTRA_LIBRARIES})
target_link_libraries(hddm-cpp PUBLIC ${EXTRA_LIBRARIES})
target_link_libraries(hddm-c PUBLIC ${EXTRA_LIBRARIES})
target_link_libraries(hddm-py PUBLIC ${EXTRA_LIBRARIES})
target_link_libraries(hddmcat PUBLIC ${EXTRA_LIBRARIES})

target_include_directories(hddm-xml PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(xml-hddm PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-cpp PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-c PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-py PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddmcat PUBLIC ${EXTRA_INCLUDE_DIRS})

if(${ROOT_FOUND})
    target_include_directories(hddm-root PUBLIC ${EXTRA_INCLUDE_DIRS}
                               "${ROOT_ROOT}/include"
                               )
    execute_process(COMMAND ${ROOT_ROOT}/bin/root-config --libs
                    OUTPUT_VARIABLE ROOTLIBRARIES 
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                   )
    target_link_libraries(hddm-root PUBLIC ${EXTRA_LIBRARIES}
                          "-Wl,-rpath -Wl,${ROOT_ROOT}/lib"
                         )
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

install(TARGETS hddm-xml DESTINATION bin)
install(TARGETS xml-hddm DESTINATION bin)
install(TARGETS hddm-cpp DESTINATION bin)
install(TARGETS hddm-c DESTINATION bin)
install(TARGETS hddm-py DESTINATION bin)
install(TARGETS hddmcat DESTINATION bin)
if(ROOT_FOUND)
    install(TARGETS hddm-root DESTINATION bin)
endif()
