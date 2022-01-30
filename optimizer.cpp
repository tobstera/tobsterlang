#include "optimizer.h"

namespace Tobsterlang::Optimizer {
auto optimize(llvm::Module& module,
              llvm::PassBuilder::OptimizationLevel optimization_level) -> void {
    if (optimization_level == llvm::PassBuilder::OptimizationLevel::O0) {
        return;
    }

    llvm::LoopAnalysisManager loop_analysis_manager;
    llvm::FunctionAnalysisManager function_analysis_manager;
    llvm::CGSCCAnalysisManager cgscc_analysis_manager;
    llvm::ModuleAnalysisManager module_analysis_manager;

    llvm::PassBuilder pass_builder;

    function_analysis_manager.registerPass(
        [&] { return pass_builder.buildDefaultAAPipeline(); });

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
    pass_builder.registerFunctionAnalyses(function_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(
        loop_analysis_manager, function_analysis_manager,
        cgscc_analysis_manager, module_analysis_manager);

    llvm::ModulePassManager pass_manager =
        pass_builder.buildPerModuleDefaultPipeline(optimization_level);

    pass_manager.run(module, module_analysis_manager);
}
}  // namespace Tobsterlang::Optimizer
