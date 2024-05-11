set(MODELS exam1x sample_mc sample_s simple1 test1)
set(CLASSES x       mc          s       a      v  )

if(HDF5_FOUND)
    list(APPEND EXTRA_LIBRARIES ${HDF5_LIBRARIES})
    list(APPEND EXTRA_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHDF5_SUPPORT=1")
endif()

list(APPEND MODEL_INCLUDE_DIRS ${PROJECT_BINARY_DIR}
                               ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${CMAKE_SOURCE_DIR}/src
                               ${CMAKE_SOURCE_DIR}/xstream/include
                               ${XercesC_INCLUDE_DIRS}
                               ${BZIP2_INCLUDE_DIRS}
                               ${ZLIB_INCLDUE_DIRS}
    )

add_custom_target(python_modules)

set(CMAKE_MSVCIDE_RUN_PATH ${CMAKE_INSTALL_PREFIX}/bin)

list(JOIN HDF5_INCLUDE_DIRS ":" HDF5_INCLUDE_PATH)
list(JOIN HDF5_LIBRARIES ":" HDF5_LIBRARY_PATH)
list(LENGTH MODELS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
   list(GET MODELS ${i} model)
   list(GET CLASSES ${i} class)
   add_custom_command(OUTPUT hddm_${class}.c
     COMMAND hddm-c ${CMAKE_CURRENT_SOURCE_DIR}/${model}.hddm
   )
   add_custom_command(OUTPUT hddm_${class}++.cpp
     COMMAND hddm-cpp ${CMAKE_CURRENT_SOURCE_DIR}/${model}.hddm
   )
   add_custom_target(python_module_${class}
     COMMAND hddm-py ${CMAKE_CURRENT_SOURCE_DIR}/${model}.hddm
     COMMAND ${CMAKE_SOURCE_DIR}/scripts/python_module_builder.sh
             HDDM_DIR=${CMAKE_INSTALL_PREFIX}
             XSTREAM_DIR=${CMAKE_SOURCE_DIR}/xstream
             HDDMSRC_DIR=${CMAKE_SOURCE_DIR}/src
             ${CMAKE_CURRENT_BINARY_DIR}/setup_hddm_${class}.py
     DEPENDS hddm_${class}++.cpp xstream
   )
   add_dependencies(python_modules python_module_${class})
   add_library(${model} STATIC hddm_${class}.c hddm_${class}++.cpp)
   target_include_directories(${model} PUBLIC ${MODEL_INCLUDE_DIRS})
   install(TARGETS ${model} DESTINATION lib)
endforeach()
