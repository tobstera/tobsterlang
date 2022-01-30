#ifndef NDEBUG
#include <llvm/IR/IRPrintingPasses.h>
#endif
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <boost/program_options.hpp>
#include <iostream>
#include "codegen.h"
#include "optimizer.h"
#include "options.h"
#include "parser.h"

auto compile_module(std::unique_ptr<llvm::Module> module,
                    std::string const& filename,
                    llvm::PassBuilder::OptimizationLevel optimization_level) {
    auto target_triple = llvm::sys::getDefaultTargetTriple();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        throw std::runtime_error(error);
    }

    auto cpu = llvm::sys::getHostCPUName();

    llvm::SubtargetFeatures feature_set;
    llvm::StringMap<bool> host_features;

    if (llvm::sys::getHostCPUFeatures(host_features)) {
        for (auto& feature : host_features) {
            feature_set.AddFeature(feature.first(), feature.second);
        }
    }

    auto features = feature_set.getString();

    llvm::TargetOptions opts;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto target_machine =
        target->createTargetMachine(target_triple, cpu, features, opts, rm);

    module->setTargetTriple(target_triple);
    module->setDataLayout(target_machine->createDataLayout());

    Tobsterlang::Optimizer::optimize(*module, optimization_level);

    std::error_code error_code;
    llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

    if (error_code) {
        throw std::runtime_error("Could not open file: " +
                                 error_code.message());
    }

    llvm::legacy::PassManager pass;
#ifndef NDEBUG
    pass.add(llvm::createPrintModulePass(llvm::outs()));
#endif

    auto filetype = llvm::CGFT_ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
        throw std::runtime_error(
            "TargetMachine can't emit a file of this type");
    }

    pass.run(*module);
    dest.flush();
}

int main(int argc, char** argv) {
    auto options = Tobsterlang::Options::parse(argc, argv);

    if (options.variables.count("help")) {
        std::cout << options.description << std::endl;

        return 0;
    }

    auto input_file = options.variables["input"].as<std::string>();

    auto tree = Tobsterlang::Parser::parse(input_file);

    Tobsterlang::Codegen codegen;
    auto module = codegen.generate(tree);

    std::string object_file =
        options.variables.count("output")
            ? options.variables["output"].as<std::string>()
            : "./" + std::string(module->getName().data()) + ".o";

    auto optimization_level = Tobsterlang::Options::get_optimization_level(
        options.variables["optimization"].as<std::string>());

    compile_module(std::move(module), object_file, optimization_level);

    return 0;
}
