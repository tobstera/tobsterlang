//
// Created by inva on 1/30/22.
//

#ifndef TOBSTERLANG_OPTIMIZER_H
#define TOBSTERLANG_OPTIMIZER_H

#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

namespace Tobsterlang::Optimizer {
auto optimize(llvm::Module& module,
              llvm::PassBuilder::OptimizationLevel optimization_level) -> void;
}

#endif  // TOBSTERLANG_OPTIMIZER_H
