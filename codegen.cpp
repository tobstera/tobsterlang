#include "codegen.h"

namespace Tobsterlang {
auto Codegen::get_function_by_name(llvm::Module& module,
                                   const std::string& name) {
    auto function = module.getFunction(name);
    if (function) {
        return function;
    }

    auto lib_func_it = m_lib_funcs.find(name);
    if (lib_func_it != m_lib_funcs.end()) {
        return lib_func_it->second(module);
    }

    throw std::runtime_error("unknown function: " + name);
}

auto Codegen::get_type_by_name(const std::string& name) {
    auto type = type_by_name[name];
    if (type) {
        return type(m_llvm_context);
    }

    throw std::runtime_error("unknown type: " + name);
}

auto Codegen::generate(ASTNode const& tree) -> std::unique_ptr<llvm::Module> {
    auto root_node = tree.get_child("Root");
    auto module_name = root_node.get_child("<xmlattr>.module").data();

    auto module = std::make_unique<llvm::Module>(module_name, m_llvm_context);

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
                llvm::Type* return_type = llvm::Type::getVoidTy(m_llvm_context);

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
                    llvm::BasicBlock::Create(m_llvm_context, "entry", func);

                m_builder.SetInsertPoint(func_body);

                m_named_values.clear();
                for (auto& arg : func->args()) {
                    auto arg_mem = m_builder.CreateAlloca(arg.getType());
                    m_builder.CreateStore(&arg, arg_mem);

                    m_named_values[arg.getName().str()] = arg_mem;
                }

                auto children = recurse_tree(subtree);

                auto has_return = subtree.rbegin()->first == "Return";
                if (!has_return) {
                    if (return_type->isVoidTy() || children.empty()) {
                        m_builder.CreateRet(nullptr);
                    } else {
                        m_builder.CreateRet(children.back());
                    }
                }

                ret.push_back(func);
            } else if (node_name == "Var") {
                // TODO: Global variables
                auto name = subtree.get_child("<xmlattr>.name").data();

                auto type_node = subtree.get_child("<xmlattr>.type");
                auto type = get_type_by_name(type_node.data());

                auto var = m_builder.CreateAlloca(type);

                // TODO: We should handle variable shadowing in the future
                m_named_values[name] = var;
            } else if (node_name == "Value") {
                auto type_name = subtree.get_child("<xmlattr>.type").data();
                auto value = Utils::unescape(subtree.data());

                auto type = get_type_by_name(type_name);
                if (type->isIntegerTy()) {
                    ret.push_back(llvm::ConstantInt::get(
                        m_llvm_context,
                        llvm::APInt(type->getIntegerBitWidth(), value, 10)));
                } else if (type_name == "String") {
                    ret.push_back(m_builder.CreateGlobalStringPtr(value));
                } else {
                    throw std::runtime_error("unknown type: " + type_name);
                }
            } else if (node_name == "Store") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = m_named_values[name];

                auto children = recurse_tree(subtree);
                assert(children.size() == 1);

                m_builder.CreateStore(children[0], var);
            } else if (node_name == "Load") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = m_named_values[name];

                ret.push_back(m_builder.CreateLoad(var));
            } else if (node_name == "Ref") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto var = m_named_values[name];

                ret.push_back(var);
            } else if (node_name == "Add") {
                auto values = recurse_tree(subtree);
                assert(values.size() >= 2);

                llvm::Value* sum = m_builder.CreateAdd(values[0], values[1]);
                for (auto i = 2; i < values.size(); ++i) {
                    sum = m_builder.CreateAdd(sum, values[i]);
                }

                ret.push_back(sum);
            } else if (node_name == "Sub") {
                auto values = recurse_tree(subtree);
                assert(values.size() >= 2);

                llvm::Value* difference =
                    m_builder.CreateSub(values[0], values[1]);
                for (auto i = 2; i < values.size(); ++i) {
                    difference = m_builder.CreateSub(difference, values[i]);
                }

                ret.push_back(difference);
            } else if (node_name == "Return") {
                auto values = recurse_tree(subtree);
                if (values.size() == 1) {
                    m_builder.CreateRet(values[0]);
                } else {
                    m_builder.CreateRet(nullptr);
                }
            } else if (node_name == "Call") {
                auto name = subtree.get_child("<xmlattr>.name").data();
                auto func = get_function_by_name(*module, name);

                auto args = recurse_tree(subtree);

                ret.push_back(m_builder.CreateCall(func, args));
            }
        }

        return ret;
    };

    recurse_tree(root_node);

    return module;
}
}  // namespace Tobsterlang