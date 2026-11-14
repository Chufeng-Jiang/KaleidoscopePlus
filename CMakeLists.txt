cmake_minimum_required(VERSION 3.20)
project(Kaleidoscope LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
if(LLVM_VERSION_MAJOR LESS 21)
    message(FATAL_ERROR "Kaleidoscope requires LLVM 21 or newer; set LLVM_DIR to its lib/cmake/llvm directory")
endif()

add_library(kaleidoscope_frontend
    src/frontend/Lexer.cpp
    src/frontend/Parser.cpp
    src/frontend/SemanticAnalyzer.cpp
    src/support/Diagnostics.cpp
    src/support/SourceManager.cpp
)
target_include_directories(kaleidoscope_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kaleidoscope_frontend PUBLIC cxx_std_17)

add_library(kaleidoscope_backend
    src/backend/llvm/Expressions.cpp
    src/backend/llvm/ControlFlow.cpp
    src/backend/llvm/Functions.cpp
    src/backend/llvm/Module.cpp
    src/backend/llvm/Variables.cpp
    src/backend/llvm/Types.cpp
    src/backend/llvm/DebugInfo.cpp
    src/backend/llvm/Optimizer.cpp
    src/backend/llvm/Output.cpp
    src/backend/llvm/KaleidoscopeJIT.cpp
    src/driver/Driver.cpp
    src/driver/CompilerOptions.cpp
)
# Internal headers are private; consumers use include/kaleidoscope only.
target_include_directories(kaleidoscope_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(kaleidoscope_backend SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(KALEIDOSCOPE_LLVM_DEFINITIONS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(kaleidoscope_backend PRIVATE ${KALEIDOSCOPE_LLVM_DEFINITIONS})
target_link_libraries(kaleidoscope_backend PUBLIC kaleidoscope_frontend PRIVATE LLVM)

add_executable(kale src/main.cpp src/runtime/Runtime.cpp)
target_link_libraries(kale PRIVATE kaleidoscope_backend)
set_target_properties(kale PROPERTIES ENABLE_EXPORTS ON)

include(CTest)
if(BUILD_TESTING)
    add_executable(frontend_test tests/frontend_test.cpp)
    target_link_libraries(frontend_test PRIVATE kaleidoscope_frontend)
    add_test(NAME frontend_sessions COMMAND frontend_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME compiler_regression
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression.py
                $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME compiler_debug_regression
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/regression.py
                $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR} -g)
    find_program(KALEIDOSCOPE_DWARFDUMP NAMES llvm-dwarfdump llvm-dwarfdump-${LLVM_VERSION_MAJOR}
                 HINTS ${LLVM_TOOLS_BINARY_DIR})
    find_program(KALEIDOSCOPE_LLDB NAMES lldb-${LLVM_VERSION_MAJOR} lldb
                 HINTS ${LLVM_TOOLS_BINARY_DIR})
    if(KALEIDOSCOPE_DWARFDUMP)
        add_test(NAME debug_info
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/debug_info_test.py
                    $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                    ${KALEIDOSCOPE_DWARFDUMP})
        if(KALEIDOSCOPE_LLDB)
            add_test(NAME debugger_integration
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/debug_info_test.py
                        $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                        ${KALEIDOSCOPE_DWARFDUMP} ${KALEIDOSCOPE_LLDB})
        else()
            message(STATUS "LLDB not found: debugger_integration test is unavailable")
        endif()
    else()
        message(STATUS "llvm-dwarfdump not found: DWARF/debugger tests are unavailable")
    endif()
endif()

if(BUILD_TESTING)
    find_program(KALEIDOSCOPE_LLVM_AS NAMES llvm-as llvm-as-${LLVM_VERSION_MAJOR} HINTS ${LLVM_TOOLS_BINARY_DIR})
    find_program(KALEIDOSCOPE_OPT NAMES opt opt-${LLVM_VERSION_MAJOR} HINTS ${LLVM_TOOLS_BINARY_DIR})
    find_program(KALEIDOSCOPE_LLC NAMES llc llc-${LLVM_VERSION_MAJOR} HINTS ${LLVM_TOOLS_BINARY_DIR})
    if(KALEIDOSCOPE_LLVM_AS AND KALEIDOSCOPE_OPT AND KALEIDOSCOPE_LLC)
        add_test(NAME compiler_pipeline
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/compiler_pipeline_test.py
                    $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER}
                    ${KALEIDOSCOPE_LLVM_AS} ${KALEIDOSCOPE_OPT} ${KALEIDOSCOPE_LLC})
    else()
        message(STATUS "LLVM command-line tools missing: compiler_pipeline test is unavailable")
    endif()
endif()

if(BUILD_TESTING)
    set(KALEIDOSCOPE_GLOBAL_TEST_ARGS)
    if(KALEIDOSCOPE_DWARFDUMP)
        list(APPEND KALEIDOSCOPE_GLOBAL_TEST_ARGS --dwarf ${KALEIDOSCOPE_DWARFDUMP})
    endif()
    if(KALEIDOSCOPE_LLVM_AS)
        list(APPEND KALEIDOSCOPE_GLOBAL_TEST_ARGS --llvm-as ${KALEIDOSCOPE_LLVM_AS})
    endif()
    add_test(NAME global_variables
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/globals_test.py
                $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                ${KALEIDOSCOPE_GLOBAL_TEST_ARGS})
    if(KALEIDOSCOPE_LLDB AND KALEIDOSCOPE_DWARFDUMP)
        add_test(NAME global_debugger
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/globals_test.py
                    $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                    --debugger-only --lldb ${KALEIDOSCOPE_LLDB} --dwarf ${KALEIDOSCOPE_DWARFDUMP})
    endif()
endif()

if(BUILD_TESTING)
    add_executable(semantic_test tests/semantic_test.cpp)
    target_link_libraries(semantic_test PRIVATE kaleidoscope_frontend)
    add_test(NAME semantic_analysis COMMAND semantic_test)
    add_test(NAME diagnostics_recovery
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/diagnostics_test.py $<TARGET_FILE:kale>)
endif()

if(BUILD_TESTING)
    add_executable(types_test tests/types_test.cpp)
    target_link_libraries(types_test PRIVATE kaleidoscope_frontend)
    add_test(NAME type_analysis COMMAND types_test)
endif()

if(BUILD_TESTING)
    add_test(NAME typed_language
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/typed_language_test.py
                $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                ${KALEIDOSCOPE_GLOBAL_TEST_ARGS})
    if(KALEIDOSCOPE_LLDB AND KALEIDOSCOPE_DWARFDUMP)
        add_test(NAME typed_debugger
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/typed_language_test.py
                    $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}
                    --debugger-only --lldb ${KALEIDOSCOPE_LLDB} --dwarf ${KALEIDOSCOPE_DWARFDUMP})
    endif()
endif()

# The original LLVM backend remains buildable without MLIR.
option(KALEIDOSCOPE_ENABLE_MLIR "Build the scalar MLIR dialect conversion exercise" OFF)
if(KALEIDOSCOPE_ENABLE_MLIR)
    # MLIRConfig also loads LLVM: require an exact match before importing targets.
    find_package(MLIR ${LLVM_PACKAGE_VERSION} EXACT REQUIRED CONFIG)
    add_subdirectory(src/backend/mlir)
endif()

add_subdirectory(src/runtime)

option(KALEIDOSCOPE_BUILD_BENCHMARKS "Build reproducible CPU/CUDA benchmark harness" OFF)
if(KALEIDOSCOPE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
