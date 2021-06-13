#ifndef CONTEXT_H
#define CONTEXT_H
#include "basis.hpp"

#include <string>
#include <fstream>
#include <list>
#include <map>
#include <iomanip>

//Module创建时需要一个context
static llvm::LLVMContext llvm_context;

namespace fpc
{
    class ArrayTypeNode;
    class RecordTypeNode;  

    class CodegenExcep : public std::exception {
    public:
        explicit CodegenExcep(const std::string &description) : description(description) {};
        const char *what() const noexcept {
            return description.c_str();
        }
    private:
        // unsigned id;
        std::string description;
    };

    //代码生成的上下文环境
    class CodegenContext
    {
    private:
    //Module_Ob模块包含了代码中的所有函数和变量
        std::unique_ptr<llvm::Module> _module;
        //Builder对象帮助生成LLVM IR 并且记录程序的当前点，以插入LLVM指令，以及Builder对象有创建新指令的函数
        llvm::IRBuilder<> builder;      
        //
        std::map<std::string, llvm::Type *> aliases;
        std::map<std::string, std::shared_ptr<ArrayTypeNode>> arrAliases;
        std::map<std::string, std::shared_ptr<std::pair<int, int>>> arrTable;
        std::map<std::string, std::shared_ptr<RecordTypeNode>> recAliases;
        std::map<std::string, llvm::Value*> locals;
        std::map<std::string, llvm::Value*> consts;
        std::map<std::string, llvm::Constant*> constVals;
        std::ofstream of;

        void createTempStr()
        {
            auto *ty = llvm::Type::getInt8Ty(llvm_context);
            llvm::Constant *z = llvm::ConstantInt::get(ty, 0);
            llvm::ArrayType* arr = llvm::ArrayType::get(ty, 256);
            std::vector<llvm::Constant *> initVector;
            for (int i = 0; i < 256; i++)
                initVector.push_back(z);
            auto *variable = llvm::ConstantArray::get(arr, initVector);

            // std::cout << "Created array" << std::endl;

            new llvm::GlobalVariable(*_module, variable->getType(), false, llvm::GlobalVariable::ExternalLinkage, variable, "__tmp_str");
        }

    public:

        bool is_subroutine;
        std::list<std::string> traces;
        llvm::Function *printfFunc, *sprintfFunc, *scanfFunc, *absFunc, *fabsFunc, *sqrtFunc, *strcpyFunc, *strcatFunc, *getcharFunc, *strlenFunc, *atoiFunc;

        std::unique_ptr<llvm::legacy::FunctionPassManager> fpm;
        std::unique_ptr<llvm::legacy::PassManager> mpm;

       // std::ofstream &log() { return of; }

        static std::string getLLVMTypeName(const llvm::Type *ty)
        {
            
            int id = ty->getTypeID();
            llvm::Type *arrTy;
            switch (id)
            {
            case 11:
                if (ty->isIntegerTy(1)) return "Boolean";
                if (ty->isIntegerTy(8)) return "Char";
                if (ty->isIntegerTy(32)) return "Integer/Long";
                return "Unknown";
            case 15: 
                if (ty->getPointerElementType()->isIntegerTy(8)) return "String";
                return "Unknown";
            case 3:
                return "Real";
            case 14:
                arrTy = ty->getArrayElementType();
                if (arrTy->isIntegerTy(8)) return "String";
                return "Array of " + getLLVMTypeName(arrTy);
            case 13:
                return "Record";
            case 12:
                return "Function";
            case 0:
                return "Void";
            default:
                return "Unknown";
            }
        }


        CodegenContext(const std::string &module_id, bool opt = false)
            : builder(llvm::IRBuilder<>(llvm_context)), _module(std::make_unique<llvm::Module>(module_id, llvm_context)), is_subroutine(false), of("compile.log")
        {
            if (of.fail())
                throw CodegenExcep("Fails to open compile log");

            createTempStr();

            auto printfTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context)}, true);
            printfFunc = llvm::Function::Create(printfTy, llvm::Function::ExternalLinkage, "printf", *_module);

            auto sprintfTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt8PtrTy(llvm_context)}, true);
            sprintfFunc = llvm::Function::Create(sprintfTy, llvm::Function::ExternalLinkage, "sprintf", *_module);

            auto scanfTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context)}, true);
            scanfFunc = llvm::Function::Create(scanfTy, llvm::Function::ExternalLinkage, "scanf", *_module);

            auto absTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt32Ty(llvm_context)}, false);
            absFunc = llvm::Function::Create(absTy, llvm::Function::ExternalLinkage, "abs", *_module);

            auto fabsTy = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm_context), {llvm::Type::getDoubleTy(llvm_context)}, false);
            fabsFunc = llvm::Function::Create(fabsTy, llvm::Function::ExternalLinkage, "fabs", *_module);

            auto sqrtTy = llvm::FunctionType::get(llvm::Type::getDoubleTy(llvm_context), {llvm::Type::getDoubleTy(llvm_context)}, false);
            sqrtFunc = llvm::Function::Create(sqrtTy, llvm::Function::ExternalLinkage, "sqrt", *_module);

            auto strcpyTy = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt8PtrTy(llvm_context)}, false);
            strcpyFunc = llvm::Function::Create(strcpyTy, llvm::Function::ExternalLinkage, "strcpy", *_module);

            auto strcatTy = llvm::FunctionType::get(llvm::Type::getInt8PtrTy(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context), llvm::Type::getInt8PtrTy(llvm_context)}, false);
            strcatFunc = llvm::Function::Create(strcatTy, llvm::Function::ExternalLinkage, "strcat", *_module);

            auto strlenTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context)}, false);
            strlenFunc = llvm::Function::Create(strlenTy, llvm::Function::ExternalLinkage, "strlen", *_module);

            auto atoiTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt8PtrTy(llvm_context)}, false);
            atoiFunc = llvm::Function::Create(atoiTy, llvm::Function::ExternalLinkage, "atoi", *_module);

            auto getcharTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), false);
            getcharFunc = llvm::Function::Create(getcharTy, llvm::Function::ExternalLinkage, "getchar", *_module);

            printfFunc->setCallingConv(llvm::CallingConv::C);
            sprintfFunc->setCallingConv(llvm::CallingConv::C);
            scanfFunc->setCallingConv(llvm::CallingConv::C);
            absFunc->setCallingConv(llvm::CallingConv::C);
            fabsFunc->setCallingConv(llvm::CallingConv::C);
            sqrtFunc->setCallingConv(llvm::CallingConv::C);
            strcpyFunc->setCallingConv(llvm::CallingConv::C);
            strcatFunc->setCallingConv(llvm::CallingConv::C);
            strlenFunc->setCallingConv(llvm::CallingConv::C);
            atoiFunc->setCallingConv(llvm::CallingConv::C);
            getcharFunc->setCallingConv(llvm::CallingConv::C);


        }
        ~CodegenContext()
        {
            if (of.is_open()) of.close();
        }

        llvm::Value *getTempStrPtr()
        {
            auto *value = _module->getGlobalVariable("__tmp_str");
            if (value == nullptr)
                throw CodegenExcep("Global temp string not found");
            llvm::Value *zero = llvm::ConstantInt::getSigned(builder.getInt32Ty(), 0);
            return builder.CreateInBoundsGEP(value, {zero, zero});
        }

        std::string getTrace() 
        {
            if (traces.empty()) return "main";
            return traces.back(); 
        }

        llvm::Value *getLocal(const std::string &key) 
        {
            auto V = locals.find(key);
            if (V == locals.end())
                return nullptr;
            return V->second;
        };
        bool setLocal(const std::string &key, llvm::Value *value) 
        {
            if (getLocal(key))
                return false;
            locals[key] = value;
            return true;
        }
        llvm::Value *getConst(const std::string &key) 
        {
            auto V = consts.find(key);
            if (V == consts.end())
                return nullptr;
            return V->second;
        };
        bool setConst(const std::string &key, llvm::Value *value) 
        {
            if (getConst(key))
                return false;
            consts[key] = value;
            return true;
        }
        llvm::Constant* getConstVal(const std::string &key) 
        {
            auto V = constVals.find(key);
            if (V == constVals.end())
                return nullptr;
            return V->second;
        };
        llvm::ConstantInt* getConstInt(const std::string &key) 
        {
            auto V = constVals.find(key);
            if (V == constVals.end())
                return nullptr;
            llvm::Constant *val = V->second;
            if (!val->getType()->isIntegerTy())
                throw CodegenExcep("Case branch must be integer type!");
            return llvm::cast<llvm::ConstantInt>(val);
        };
        bool setConstVal(const std::string &key, llvm::Constant* value) 
        {
            if (getConstVal(key))
                return false;
            constVals[key] = value;
            return true;
        }
        std::shared_ptr<std::pair<int, int>> getArrayEntry(const std::string &key) 
        {
            auto V = arrTable.find(key);
            if (V == arrTable.end())
                return nullptr;
            return V->second;
        }
        bool setArrayEntry(const std::string &key, const std::shared_ptr<std::pair<int, int>> &value) 
        {
            if (getArrayEntry(key))
                return false;
            assert(value != nullptr);
            arrTable[key] = value;
            return true;
        }
        bool setArrayEntry(const std::string &key, const std::shared_ptr<ArrayTypeNode> &arr) 
        {
            if (getArrayEntry(key))
                return false;
            assert(arr != nullptr);
            auto st = arr->range_start->codegen(*this), ed = arr->range_end->codegen(*this);
            int s = llvm::cast<llvm::ConstantInt>(st)->getSExtValue(), 
                e = llvm::cast<llvm::ConstantInt>(ed)->getSExtValue();
            auto value = std::make_shared<std::pair<int, int>>(s, e);
            arrTable[key] = value;
            return true;
        }
        bool setArrayEntry(const std::string &key, const int start, const int end) 
        {
            if (getArrayEntry(key))
                return false;
            assert(start <= end);
            auto value = std::make_shared<std::pair<int, int>>(start, end);
            arrTable[key] = value;
            return true;
        }
        std::shared_ptr<ArrayTypeNode> getArrayAlias(const std::string &key) 
        {
            auto V = arrAliases.find(key);
            if (V == arrAliases.end())
                return nullptr;
            return V->second;
        }
        bool setArrayAlias(const std::string &key, const std::shared_ptr<ArrayTypeNode> &value) 
        {
            if (getArrayAlias(key))
                return false;
            assert(value != nullptr);
            arrAliases[key] = value;
            return true;
        }
        std::shared_ptr<RecordTypeNode> getRecordAlias(const std::string &key) 
        {
            auto V = recAliases.find(key);
            if (V == recAliases.end())
                return nullptr;
            return V->second;
        }
        bool setRecordAlias(const std::string &key, const std::shared_ptr<RecordTypeNode> &value) 
        {
            if (getRecordAlias(key))
                return false;
            assert(value != nullptr);
            recAliases[key] = value;
            return true;
        }
        llvm::Type *getAlias(const std::string &key) 
        {
            auto V = aliases.find(key);
            if (V == aliases.end())
                return nullptr;
            return V->second;
        }
        bool setAlias(const std::string &key, llvm::Type *value) 
        {
            if (getAlias(key))
                return false;
            aliases[key] = value;
            return true;
        }
        llvm::IRBuilder<> &getBuilder()
        {
            return builder;
        }
        std::unique_ptr<llvm::Module> &getModule() {
            return _module;
        }
        

    };
    
    
} 


#endif

