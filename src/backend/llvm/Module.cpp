#include "CodegenContext.h"

namespace kaleidoscope {

Value *CodegenContext::LogErrorV(const char *Str) {
    Diag.error(Str, CurrentLocation);
    return nullptr;
}

void CodegenContext::initializeModule() {
    TheContext = std::make_unique<LLVMContext>();
    TheModule = std::make_unique<Module>("Kaleidoscope", *TheContext);
    if (FileMode) {
        TheModule->setDataLayout(TheTM->createDataLayout());
        TheModule->setTargetTriple(TheTM->getTargetTriple());
    } else
        TheModule->setDataLayout(TheJIT->getDataLayout());
    Builder = std::make_unique<IRBuilder<>>(*TheContext);
    if (EmitDebugInfo)
        Debug = std::make_unique<DebugInfo>(*TheModule, SourceFile);
}

} // namespace kaleidoscope
