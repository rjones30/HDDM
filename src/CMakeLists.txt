if(HDF5_FOUND)
    list(APPEND EXTRA_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
    list(APPEND EXTRA_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})
    SET(HDF5_SUPPORT 1)
endif()

if(ROOTSYS)
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

if(WIN32)
    list(APPEND EXTRA_LIBRARIES wsock32 
                                ws2_32
                                ${CMAKE_INSTALL_PREFIX}/lib/libpthreadVC3.lib
    )
endif()

# add the executables
add_executable(hddm-xml hddm-xml.cpp XString.cpp XParsers.cpp md5.c)
add_executable(xml-hddm xml-hddm.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-cpp hddm-cpp.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-c hddm-c.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddm-py hddm-py.cpp XString.cpp XParsers.cpp md5.c)
add_executable(hddmcat hddmcat.cpp)
if(${ROOTSYS})
    add_executable(hddm-root hddm-root.cpp XString.cpp XParsers.cpp md5.c)
endif()

configure_file(VersionConfig.hpp.in VersionConfig.hpp)


if(APPLE)
    target_link_libraries(hddm-xml PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
    target_link_libraries(xml-hddm PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
    target_link_libraries(hddm-cpp PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
    target_link_libraries(hddm-c PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
    target_link_libraries(hddm-py PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
    target_link_libraries(hddmcat PUBLIC ${EXTRA_LIBRARIES}
                          "-framework CoreServices" "-lcurl")
else()
    target_link_libraries(hddm-xml PUBLIC ${EXTRA_LIBRARIES})
    target_link_libraries(xml-hddm PUBLIC ${EXTRA_LIBRARIES})
    target_link_libraries(hddm-cpp PUBLIC ${EXTRA_LIBRARIES})
    target_link_libraries(hddm-c PUBLIC ${EXTRA_LIBRARIES})
    target_link_libraries(hddm-py PUBLIC ${EXTRA_LIBRARIES})
    target_link_libraries(hddmcat PUBLIC ${EXTRA_LIBRARIES})
endif(APPLE)

target_include_directories(hddm-xml PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(xml-hddm PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-cpp PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-c PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddm-py PUBLIC ${EXTRA_INCLUDE_DIRS})
target_include_directories(hddmcat PUBLIC ${EXTRA_INCLUDE_DIRS})

if(${ROOTSYS})
    target_include_directories(hddm-root PUBLIC ${EXTRA_INCLUDE_DIRS}
	                       "${ROOTSYS}/include"
                               )
    execute_process(COMMAND ${ROOTSYS}/bin/root-config --libs
                    OUTPUT_VARIABLE ROOT_LIBRARIES 
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                   )
    target_link_libraries(hddm-root PUBLIC ${EXTRA_LIBRARIES}
	                  "-Wl,-rpath -Wl,${ROOTSYS}/lib"
                         )
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

install(FILES particleType.h DESTINATION include)
install(FILES unistd_win32.h DESTINATION include)
install(TARGETS hddm-xml DESTINATION bin)
install(TARGETS xml-hddm DESTINATION bin)
install(TARGETS hddm-cpp DESTINATION bin)
install(TARGETS hddm-c DESTINATION bin)
install(TARGETS hddm-py DESTINATION bin)
install(TARGETS hddmcat DESTINATION bin)
if(ROOTSYS)
    install(TARGETS hddm-root DESTINATION bin)
endif()
