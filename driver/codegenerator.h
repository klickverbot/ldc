//===-- driver/codegenerator.h - D module codegen entry point ---*- C++ -*-===//
//
//                         LDC – the LLVM D compiler
//
// This file is distributed under the BSD-style LDC license. See the LICENSE
// file for details.
//
//===----------------------------------------------------------------------===//
//
// Declares ldc::CodeGenerator, the main entry point for emitting D modules as
// LLVM IR and object files.
//
//===----------------------------------------------------------------------===//

#ifndef LDC_DRIVER_CODEGENERATOR_H
#define LDC_DRIVER_CODEGENERATOR_H

#include "gen/irstate.h"

namespace ldc {

/// The main entry point for emitting LLVM IR for one or more D modules and
/// subsequently storing it in whatever IR/object file/assembly format has
/// been chosen globally.
///
/// Currently reads parts of the configuration from global.params, as the code
/// has been extracted straight out of main(). This should be cleaned up in
/// the future.
///
/// Note that this class is only concerned with code generation, and not the
/// system linker step that is necessary for emitting executables.
class CodeGenerator {
public:
  CodeGenerator(llvm::LLVMContext &context, bool singleObj);
  ~CodeGenerator();
  void emit(Module *m);

private:
  void prepareLLModule(Module *m);
  void finishLLModule(Module *m);
  void writeAndFreeLLModule(const char *filename);

  llvm::LLVMContext &context_;
  int moduleCount_;
  bool const singleObj_;
  IRState *ir_;
  const char *firstModuleObjfileName_;
};
}

#endif
