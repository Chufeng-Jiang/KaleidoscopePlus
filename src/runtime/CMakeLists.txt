option(KALEIDOSCOPE_ENABLE_CUDA "Enable CUDA Driver API backend for teaching runtime" OFF)
add_library(kale_device_runtime DeviceRuntime.cpp)
target_include_directories(kale_device_runtime PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(kale_device_runtime PUBLIC cxx_std_17)
if(KALEIDOSCOPE_ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_compile_definitions(kale_device_runtime PRIVATE KALE_HAS_CUDA=1)
    target_link_libraries(kale_device_runtime PRIVATE CUDA::cuda_driver)
endif()

if(BUILD_TESTING)
    add_executable(device_runtime_test ${PROJECT_SOURCE_DIR}/tests/device_runtime_test.cpp)
    target_link_libraries(device_runtime_test PRIVATE kale_device_runtime)
    add_test(NAME device_runtime_cpu COMMAND device_runtime_test cpu)
endif()
if(KALEIDOSCOPE_ENABLE_MLIR)
    set(vector_object ${CMAKE_CURRENT_BINARY_DIR}/vector.o)
    add_custom_command(OUTPUT ${vector_object}
        COMMAND $<TARGET_FILE:kale-mlir> --emit-object
            ${PROJECT_SOURCE_DIR}/examples/Vector.kale -o ${vector_object}
        DEPENDS kale-mlir ${PROJECT_SOURCE_DIR}/examples/Vector.kale VERBATIM)
    set_source_files_properties(${vector_object} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    add_executable(kale-vector-demo ${PROJECT_SOURCE_DIR}/examples/VectorDriver.cpp ${vector_object})
    target_link_libraries(kale-vector-demo PRIVATE kale_device_runtime)
    if(BUILD_TESTING)
        add_test(NAME vector_cpu COMMAND kale-vector-demo cpu)
    endif()
    if(KALEIDOSCOPE_ENABLE_CUDA)
        set(vector_ptx ${CMAKE_CURRENT_BINARY_DIR}/vector.ptx)
        add_custom_command(OUTPUT ${vector_ptx}
            COMMAND $<TARGET_FILE:kale-mlir> --emit-ptx --gpu-map element
                ${PROJECT_SOURCE_DIR}/examples/Vector.kale -o ${vector_ptx}
            DEPENDS kale-mlir ${PROJECT_SOURCE_DIR}/examples/Vector.kale VERBATIM)
        add_custom_target(kale-vector-ptx ALL DEPENDS ${vector_ptx})
        add_dependencies(kale-vector-demo kale-vector-ptx)
        if(BUILD_TESTING)
            add_dependencies(device_runtime_test kale-vector-ptx)
            add_test(NAME vector_cuda COMMAND kale-vector-demo cuda ${vector_ptx})
            add_test(NAME device_runtime_cuda COMMAND device_runtime_test cuda ${vector_ptx})
            set_tests_properties(vector_cuda device_runtime_cuda PROPERTIES SKIP_RETURN_CODE 77 LABELS gpu)
        endif()
    endif()
endif()

if(KALEIDOSCOPE_ENABLE_MLIR)
    set(acc_object ${CMAKE_CURRENT_BINARY_DIR}/acc-vector.o)
    add_custom_command(OUTPUT ${acc_object}
        COMMAND $<TARGET_FILE:kale-mlir> --emit-object
            ${PROJECT_SOURCE_DIR}/examples/AccVector.kale -o ${acc_object}
        DEPENDS kale-mlir ${PROJECT_SOURCE_DIR}/examples/AccVector.kale VERBATIM)
    set_source_files_properties(${acc_object} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    add_executable(kale-acc-demo ${PROJECT_SOURCE_DIR}/examples/AccDriver.cpp ${acc_object})
    target_link_libraries(kale-acc-demo PRIVATE kale_device_runtime)
    if(BUILD_TESTING)
        add_test(NAME acc_directives COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/acc_directive_test.py
            $<TARGET_FILE:kale-mlir> $<TARGET_FILE:kale> ${PROJECT_SOURCE_DIR})
        add_test(NAME acc_cpu COMMAND kale-acc-demo cpu)
    endif()
    if(KALEIDOSCOPE_ENABLE_CUDA)
        set(acc_ptx ${CMAKE_CURRENT_BINARY_DIR}/acc-vector.ptx)
        add_custom_command(OUTPUT ${acc_ptx}
            COMMAND $<TARGET_FILE:kale-mlir> --emit-ptx
                ${PROJECT_SOURCE_DIR}/examples/AccVector.kale -o ${acc_ptx}
            DEPENDS kale-mlir ${PROJECT_SOURCE_DIR}/examples/AccVector.kale VERBATIM)
        add_custom_target(kale-acc-ptx ALL DEPENDS ${acc_ptx})
        add_dependencies(kale-acc-demo kale-acc-ptx)
        if(BUILD_TESTING)
            add_test(NAME acc_cuda COMMAND kale-acc-demo cuda ${acc_ptx})
            set_tests_properties(acc_cuda PROPERTIES SKIP_RETURN_CODE 77 LABELS gpu)
        endif()
    endif()
endif()
