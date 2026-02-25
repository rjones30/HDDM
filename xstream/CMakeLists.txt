add_subdirectory(src)

target_include_directories(xstream INTERFACE 
    $<INSTALL_INTERFACE:include>
)

install(TARGETS xstream
        EXPORT HDDMTargets
        DESTINATION lib)

install(FILES include/xstream.h DESTINATION include)
install(DIRECTORY include/xstream DESTINATION include)
