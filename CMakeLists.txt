cmake_minimum_required(VERSION 3.20)
project(Kaleidoscope LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
if(LLVM_VERSION_MAJOR LESS 21)
    message(FATAL_ERROR "Kaleidoscope requires LLVM 21 or newer; set LLVM_DIR to its lib/cmake/llvm directory")
endif()

add_library(kaleidoscope_frontend src/Lexer.cpp src/Parser.cpp src/Diagnostics.cpp)
target_include_directories(kaleidoscope_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(kaleidoscope_frontend PUBLIC cxx_std_17)

add_library(kaleidoscope_backend src/Codegen.cpp src/DebugInfo.cpp src/Driver.cpp src/KaleidoscopeJIT.cpp
    src/CompilerOptions.cpp src/Optimizer.cpp src/Output.cpp src/Globals.cpp)
target_include_directories(kaleidoscope_backend SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(KALEIDOSCOPE_LLVM_DEFINITIONS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(kaleidoscope_backend PRIVATE ${KALEIDOSCOPE_LLVM_DEFINITIONS})
target_link_libraries(kaleidoscope_backend PUBLIC kaleidoscope_frontend PRIVATE LLVM)

add_executable(kale src/main.cpp src/Runtime.cpp)
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
