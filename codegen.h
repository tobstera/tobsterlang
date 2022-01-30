#ifndef TOBSTERLANG_CODEGEN_H
#define TOBSTERLANG_CODEGEN_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <unordered_map>
#include "parser.h"
#include "utils.h"

namespace Tobsterlang {

class Codegen {
public:
    typedef llvm::Type* (*type_factory_t)(llvm::LLVMContext&);

    auto generate(ASTNode const&) -> std::unique_ptr<llvm::Module>;

private:
    auto get_function_by_name(llvm::Module&, std::string const&);
    auto get_type_by_name(std::string const&);

    llvm::LLVMContext m_llvm_context{};
    llvm::IRBuilder<> m_builder{m_llvm_context};
    std::unordered_map<std::string, llvm::AllocaInst*> m_named_values;

    std::unordered_map<std::string, type_factory_t> type_by_name{
        {"Void", reinterpret_cast<type_factory_t>(llvm::Type::getVoidTy)},
        {"Int8", reinterpret_cast<type_factory_t>(llvm::Type::getInt8Ty)},
        {"Int16", reinterpret_cast<type_factory_t>(llvm::Type::getInt16Ty)},
        {"Int32", reinterpret_cast<type_factory_t>(llvm::Type::getInt32Ty)},
        {"Int64", reinterpret_cast<type_factory_t>(llvm::Type::getInt64Ty)},
        {"String", reinterpret_cast<type_factory_t>(llvm::Type::getInt8PtrTy)},
    };

    std::unordered_map<std::string,
                       std::function<llvm::Function*(llvm::Module&)>>
        m_lib_funcs{
            {"printf",
             [this](llvm::Module& module) {
                 auto printf_args = std::vector<llvm::Type*>{
                     llvm::Type::getInt8PtrTy(m_llvm_context)};

                 auto printf_type = llvm::FunctionType::get(
                     llvm::Type::getInt32Ty(m_llvm_context), printf_args, true);

                 auto printf_func = llvm::Function::Create(
                     printf_type, llvm::Function::ExternalLinkage, "printf",
                     module);

                 printf_func->setCallingConv(llvm::CallingConv::C);

                 return printf_func;
             }},
            {"scanf", [this](llvm::Module& module) {
                 auto scanf_args = std::vector<llvm::Type*>{
                     llvm::Type::getInt8PtrTy(m_llvm_context)};

                 auto scanf_type = llvm::FunctionType::get(
                     llvm::Type::getInt32Ty(m_llvm_context), scanf_args, true);

                 auto scanf_func = llvm::Function::Create(
                     scanf_type, llvm::Function::ExternalLinkage, "scanf",
                     module);

                 scanf_func->setCallingConv(llvm::CallingConv::C);

                 return scanf_func;
             }}};
};
}  // namespace Tobsterlang

#endif  // TOBSTERLANG_CODEGEN_H
