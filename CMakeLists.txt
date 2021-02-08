cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(HDDM VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add directories to the include list
list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/xstream/include)
include_directories(PUBLIC ${EXTRA_INCLUDES})

# add the xstream library
add_subdirectory(xstream)

# add the libraries to the link list
list(APPEND EXTRA_LIBS xstream)

# build the executables
add_subdirectory(src)
