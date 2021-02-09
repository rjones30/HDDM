add_subdirectory(src)

install(TARGETS xstream DESTINATION lib64)
install(FILES include/xstream.h DESTINATION include)
install(DIRECTORY include/xstream DESTINATION include)
