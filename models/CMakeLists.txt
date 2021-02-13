set(MODELS exam1x sample_mc sample_s simple1 test1)
set(CLASSES x       mc          s       a      v  )

list(APPEND MODEL_INCLUDE_DIRS ${PROJECT_BINARY_DIR}
                               ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}
                               ${CMAKE_SOURCE_DIR}/src
                               ${CMAKE_SOURCE_DIR}/xstream/include
                               ${XercesC_INCLUDE_DIRS}
                               ${BZIP2_INCLUDE_DIRS}
                               ${ZLIB_INCLDUE_DIRS}
    )

add_custom_target(python_module_install ALL)

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
     COMMAND python setup_hddm_${class}.py
     DEPENDS hddm_${class}++.cpp
   )
   add_dependencies(python_module_install python_module_${class})
   add_library(lib${model} STATIC hddm_${class}.c hddm_${class}++.cpp)
   target_include_directories(lib${model} PUBLIC ${MODEL_INCLUDE_DIRS})
   install(TARGETS lib${model} DESTINATION lib64)
endforeach()
