//
// Created by inva on 1/30/22.
//

#ifndef TOBSTERLANG_COMPILER_H
#define TOBSTERLANG_COMPILER_H

#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>
#include <memory>
#include <string>

namespace Tobsterlang::Compiler {
auto compile(std::unique_ptr<llvm::Module> module,
             std::string const& filename,
             llvm::PassBuilder::OptimizationLevel optimization_level) -> void;
}

#endif  // TOBSTERLANG_COMPILER_H
