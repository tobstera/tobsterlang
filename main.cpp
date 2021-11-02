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
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>

static llvm::LLVMContext llvm_context;
static llvm::IRBuilder<> builder(llvm_context);

// Unescapes a string literal.
auto unescape(std::string const& str) {
    std::string result;
    result.reserve(str.size());

    for (auto i = 0u; i < str.size(); ++i) {
        if (str[i] == '\\') {
            switch (str[++i]) {
            case '\\':
                result += '\\';
                break;
            case '\'':
                result += '\'';
                break;
            case '\"':
                result += '\"';
                break;
            case '0':
                result += '\0';
                break;
            case 'a':
                result += '\a';
                break;
            case 'b':
                result += '\b';
                break;
            case 'f':
                result += '\f';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            case 'v':
                result += '\v';
                break;
            default:
                std::cerr << "Unknown escape sequence: \\" << str[i]
                          << std::endl;
                break;
            }
        } else {
            result += str[i];
        }
    }

    return result;
}

auto parse_program(std::string const& filename) {
    boost::property_tree::ptree tree;
    boost::property_tree::read_xml(filename, tree);

    return tree;
}

auto compile_program(boost::property_tree::ptree const& tree) {
    auto root_node = tree.get_child("Root");
    auto module_name = root_node.get_child("<xmlattr>.module").data();

    auto module = std::make_unique<llvm::Module>(module_name, llvm_context);

    std::function<std::vector<llvm::Value*>(boost::property_tree::ptree const&)>
        recurse_tree = [&](boost::property_tree::ptree const& tree)
        -> std::vector<llvm::Value*> {
        if (tree.empty()) {
            return {};
        }

        std::vector<llvm::Value*> ret;

        for (auto& node : tree) {
            if (node.first == "Func") {
                auto func_name = node.second.get_child("<xmlattr>.name").data();
                if (func_name == "ZdraveitePriqteliAzSumTobstera") {
                    func_name = "main";
                }

                // TODO: Figure out how to define arguments
                auto func_type = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(llvm_context), {}, false);
                auto func = llvm::Function::Create(
                    func_type, llvm::Function::ExternalLinkage, func_name,
                    module.get());

                auto func_body =
                    llvm::BasicBlock::Create(llvm_context, "entry", func);

                builder.SetInsertPoint(func_body);

                recurse_tree(node.second);

                builder.CreateRet(nullptr);

                ret.push_back(func);
            } else if (node.first == "Print") {
                auto format = node.second.get_child("<xmlattr>.format").data();
                auto format_str =
                    builder.CreateGlobalStringPtr(unescape(format));

                auto printf_func = module->getFunction("printf");
                if (!printf_func) {
                    auto printf_args = std::vector<llvm::Type*>{
                        llvm::Type::getInt8PtrTy(llvm_context)};

                    auto printf_type = llvm::FunctionType::get(
                        llvm::Type::getInt32Ty(llvm_context), printf_args,
                        true);

                    printf_func = llvm::Function::Create(
                        printf_type, llvm::Function::ExternalLinkage, "printf",
                        module.get());

                    printf_func->setCallingConv(llvm::CallingConv::C);
                }

                auto printf_call_args = recurse_tree(node.second);
                printf_call_args.insert(printf_call_args.begin(), format_str);

                builder.CreateCall(printf_func, printf_call_args);
            } else if (node.first == "String") {
                auto value = node.second.data();
                auto value_str = builder.CreateGlobalStringPtr(value);

                ret.push_back(value_str);
            }
        }

        return ret;
    };

    recurse_tree(root_node);

    return module;
}

auto optimize_module(llvm::Module& module) {
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
        pass_builder.buildPerModuleDefaultPipeline(
            llvm::PassBuilder::OptimizationLevel::O3);

    pass_manager.run(module, module_analysis_manager);
}

auto compile_module(std::unique_ptr<llvm::Module> module,
                    std::string const& filename) {
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

    optimize_module(*module);

    std::error_code error_code;
    llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

    if (error_code) {
        throw std::runtime_error("Could not open file: " +
                                 error_code.message());
    }

    llvm::legacy::PassManager pass;
    auto filetype = llvm::CGFT_ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, filetype)) {
        throw std::runtime_error(
            "TargetMachine can't emit a file of this type");
    }

    pass.run(*module);
    dest.flush();
}

int main() {
    auto tree = parse_program("../hello.xml");
    auto module = compile_program(tree);

    std::string object_file =
        "../" + std::string(module->getName().data()) + ".o";

    compile_module(std::move(module), object_file);

    return 0;
}
