list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}" "${LLVM_CMAKE_DIR}")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include_directories(SYSTEM ${MLIR_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
set(LLVM_TARGET_DEFINITIONS KaleOps.td)
mlir_tablegen(KaleOps.h.inc -gen-op-decls)
mlir_tablegen(KaleOps.cpp.inc -gen-op-defs)
mlir_tablegen(KaleDialect.h.inc -gen-dialect-decls -dialect=kale)
mlir_tablegen(KaleDialect.cpp.inc -gen-dialect-defs -dialect=kale)
add_public_tablegen_target(KaleOpsIncGen)

add_library(kaleidoscope_mlir KaleDialect.cpp LowerToArith.cpp LowerToLLVM.cpp AccDirective.cpp MLIRGen.cpp Export.cpp
    ${PROJECT_SOURCE_DIR}/src/backend/llvm/Optimizer.cpp)
add_dependencies(kaleidoscope_mlir KaleOpsIncGen)
target_compile_features(kaleidoscope_mlir PUBLIC cxx_std_17)
target_include_directories(kaleidoscope_mlir PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src)
target_include_directories(kaleidoscope_mlir SYSTEM PUBLIC ${MLIR_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_link_libraries(kaleidoscope_mlir PUBLIC kaleidoscope_frontend
    MLIRIR MLIRArithDialect MLIRFuncDialect MLIRSCFDialect MLIRMemRefDialect MLIRControlFlowDialect
    MLIRPass MLIRTransformUtils MLIRArithToLLVM MLIRFuncToLLVM MLIRMemRefToLLVM
    MLIRControlFlowToLLVM MLIRSCFToControlFlow MLIRLLVMCommonConversion
    MLIRTargetLLVMIRExport MLIRLLVMToLLVMIRTranslation MLIRBuiltinToLLVMIRTranslation MLIRParser LLVM)

add_executable(kale-mlir ${PROJECT_SOURCE_DIR}/src/tools/kale-mlir.cpp)
target_link_libraries(kale-mlir PRIVATE kaleidoscope_mlir)
add_executable(kale-opt ${PROJECT_SOURCE_DIR}/src/tools/kale-opt.cpp)
target_link_libraries(kale-opt PRIVATE kaleidoscope_mlir MLIROptLib)

if(BUILD_TESTING)
    add_test(NAME mlir_execution
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/mlir_execution_test.py
            $<TARGET_FILE:kale-mlir> $<TARGET_FILE:kale> ${CMAKE_CXX_COMPILER} ${PROJECT_SOURCE_DIR}
            ${LLVM_TOOLS_BINARY_DIR}/llvm-as)
    add_executable(mlir_missing_pattern_test ${PROJECT_SOURCE_DIR}/tests/mlir_missing_pattern_test.cpp)
    target_link_libraries(mlir_missing_pattern_test PRIVATE kaleidoscope_mlir MLIRParser)
    add_test(NAME mlir_missing_pattern COMMAND mlir_missing_pattern_test)
    add_test(NAME mlir_conversion
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/mlir_conversion_test.py
            $<TARGET_FILE:kale-mlir> $<TARGET_FILE:kale-opt>)
endif()
