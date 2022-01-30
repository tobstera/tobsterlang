#ifndef NDEBUG
#include <llvm/IR/IRPrintingPasses.h>
#endif
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/IRBuilder.h>
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
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>
#include "options.h"
#include "parser.h"
#include "utils.h"

namespace po = boost::program_options;

static llvm::LLVMContext llvm_context;
static llvm::IRBuilder<> builder(llvm_context);

static std::unordered_map<std::string, llvm::AllocaInst*> named_values;

typedef llvm::Type* (*type_factory_t)(llvm::LLVMContext&);

auto type_by_name = std::unordered_map<std::string, type_factory_t>{
    {"Void", reinterpret_cast<type_factory_t>(llvm::Type::getVoidTy)},
    {"Int8", reinterpret_cast<type_factory_t>(llvm::Type::getInt8Ty)},
    {"Int16", reinterpret_cast<type_factory_t>(llvm::Type::getInt16Ty)},
    {"Int32", reinterpret_cast<type_factory_t>(llvm::Type::getInt32Ty)},
    {"Int64", reinterpret_cast<type_factory_t>(llvm::Type::getInt64Ty)},
    {"String", reinterpret_cast<type_factory_t>(llvm::Type::getInt8PtrTy)},
};

auto lib_funcs =
    std::unordered_map<std::string,
                       std::function<llvm::Function*(llvm::Module&)>>{
        {"printf",
         [](llvm::Module& module) {
             auto printf_args = std::vector<llvm::Type*>{
                 llvm::Type::getInt8PtrTy(llvm_context)};

             auto printf_type = llvm::FunctionType::get(
                 llvm::Type::getInt32Ty(llvm_context), printf_args, true);

             auto printf_func = llvm::Function::Create(
                 printf_type, llvm::Function::ExternalLinkage, "printf",
                 module);

             printf_func->setCallingConv(llvm::CallingConv::C);

             return printf_func;
         }},
        {"scanf", [](llvm::Module& module) {
             auto scanf_args = std::vector<llvm::Type*>{
                 llvm::Type::getInt8PtrTy(llvm_context)};

             auto scanf_type = llvm::FunctionType::get(
                 llvm::Type::getInt32Ty(llvm_context), scanf_args, true);

             auto scanf_func = llvm::Function::Create(
                 scanf_type, llvm::Function::ExternalLinkage, "scanf", module);

             scanf_func->setCallingConv(llvm::CallingConv::C);

             return scanf_func;
         }}};

auto get_type_by_name(std::string const& name) -> llvm::Type* {
    auto type = type_by_name[name];
    if (type) {
        return type(llvm_context);
    }

    throw std::runtime_error("unknown type: " + name);
}

auto get_function_by_name(llvm::Module& module, std::string const& name) {
    auto function = module.getFunction(name);
    if (function) {
        return function;
    }

    auto lib_func_it = lib_funcs.find(name);
    if (lib_func_it != lib_funcs.end()) {
        return lib_func_it->second(module);
    }

    throw std::runtime_error("unknown function: " + name);
}

auto compile_program(Tobsterlang::ASTNode const& tree) {
    auto root_node = tree.get_child("Root");
    auto module_name = root_node.get_child("<xmlattr>.module").data();

    auto module = std::make_unique<llvm::Module>(module_name, llvm_context);

    std::function<std::vector<llvm::Value*>(Tobsterlang::ASTNode const&)>
        recurse_tree =
            [&](Tobsterlang::ASTNode const& tree) -> std::vector<llvm::Value*> {
        if (tree.empty()) {
            return {};
        }

        std::vector<llvm::Value*> ret;

        for (auto& [node_name, subtree] : tree) {
            if (node_name == "Func") {
                std::string func_name;
                llvm::Type* return_type = llvm::Type::getVoidTy(llvm_context);

                std::vector<std::string> argument_names;
                std::vector<llvm::Type*> argument_types;

                for (auto& [argument_name, argument_node] :
                     subtree.get_child("<xmlattr>")) {
                    auto argument_value = argument_node.data();

                    if (argument_name == "name") {
                        func_name = argument_value;

                        continue;
                    }

                    if (argument_name == "returns") {
                        return_type = get_type_by_name(argument_value);

                        continue;
                    }

                    auto argument_type = get_type_by_name(argument_value);

                    argument_names.push_back(argument_name);
                    argument_types.push_back(argument_type);
                }

                if (func_name == "ZdraveitePriqteliAzSumTobstera") {
                    func_name = "main";
                }

                auto func_type =
                    llvm::FunctionType::get(return_type, argument_types, false);

                auto func = llvm::Function::Create(
                    func_type, llvm::Function::ExternalLinkage, func_name,
                    module.get());

                for (auto i = 0; i < argument_names.size(); ++i) {
                    func->getArg(i)->setName(argument_names[i]);
                }

                auto func_body =
                    llvm::BasicBlock::Create(llvm_context, "entry", func);

                builder.SetInsertPoint(func_body);

                named_values.clear();
                for (auto& arg : func->args()) {
                    auto arg_mem = builder.CreateAlloca(arg.getType());
                    builder.CreateStore(&arg, arg_mem);

                    named_values[arg.getName().str()] = arg_mem;
                }

                auto children = recurse_tree(subtree);

                auto has_return = subtree.rbegin()->first == "Return";
                if (!has_return) {
                    if (return_type->isVoidTy() || children.empty()) {
                        builder.CreateRet(nullptr);
                    } else {
                        builder.CreateRet(children.back());
                    }
                }

                ret.push_back(func);
            } else if (node_name == "Var") {
                // TODO: Global variables
                auto name = subtree.get_child("<xmlattr>.name").data();

                auto type_node = subtree.get_child("<xmlattr>.type");
                auto type = get_type_by_name(type_node.data());

                auto var = builder.CreateAlloca(type);

                // TODO: We should handle variable shadowing in the future
                named_values[name] = var;
            } else if (node_name == "Value") {
                auto type_name = subtree.get_child("<xmlattr>.type").data();
                auto value = Tobsterlang::Utils::unescape(subtree.data());

                auto type = get_type_by_name(type_name);
                if (type->isIntegerTy()) {
                    ret.push_back(llvm::ConstantInt::get(
                        llvm_context,
                        llvm::APInt(type->getIntegerBitWidth(), value, 10)));
                } else if (type_name == "String") {
                    ret.push_back(builder.CreateGlobalStringPtr(value));
                } else {
                    throw std::runtime_error("unknown type: " + type_name);
                }
            } else if (node_name == "Store") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = named_values[name];

                auto children = recurse_tree(subtree);
                assert(children.size() == 1);

                builder.CreateStore(children[0], var);
            } else if (node_name == "Load") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = named_values[name];

                ret.push_back(builder.CreateLoad(var));
            } else if (node_name == "Ref") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = named_values[name];

                ret.push_back(var);
            } else if (node_name == "Add") {
                auto values = recurse_tree(subtree);
                assert(values.size() >= 2);

                llvm::Value* sum = builder.CreateAdd(values[0], values[1]);
                for (auto i = 2; i < values.size(); ++i) {
                    sum = builder.CreateAdd(sum, values[i]);
                }

                ret.push_back(sum);
            } else if (node_name == "Sub") {
                auto values = recurse_tree(subtree);
                assert(values.size() >= 2);

                llvm::Value* difference =
                    builder.CreateSub(values[0], values[1]);
                for (auto i = 2; i < values.size(); ++i) {
                    difference = builder.CreateSub(difference, values[i]);
                }

                ret.push_back(difference);
            } else if (node_name == "Return") {
                auto values = recurse_tree(subtree);
                if (values.size() == 1) {
                    builder.CreateRet(values[0]);
                } else {
                    builder.CreateRet(nullptr);
                }
            } else if (node_name == "Call") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto func = get_function_by_name(*module, name);

                auto args = recurse_tree(subtree);

                ret.push_back(builder.CreateCall(func, args));
            }
        }

        return ret;
    };

    recurse_tree(root_node);

    return module;
}

auto optimize_module(llvm::Module& module,
                     llvm::PassBuilder::OptimizationLevel optimization_level) {
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

    optimize_module(*module, optimization_level);

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

    Tobsterlang::Parser parser;
    auto tree = parser.parse(input_file);

    auto module = compile_program(tree);

    std::string object_file =
        options.variables.count("output")
            ? options.variables["output"].as<std::string>()
            : "./" + std::string(module->getName().data()) + ".o";

    auto optimization_level = Tobsterlang::Options::get_optimization_level(
        options.variables["optimization"].as<std::string>());

    compile_module(std::move(module), object_file, optimization_level);

    return 0;
}
