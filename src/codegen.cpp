#include "basis.hpp"
#include "context.hpp"
#include <cassert>

namespace fpc
{
    
    llvm::Value *ProgramNode::codegen(CodegenContext &context)
    {
        //添加main函数
        //function中添加basic block
        //创建BasicBlock可以使用BasicBlock类的静态函数Create
        context.is_subroutine = false;
        context.log() << "Entering main program" << std::endl;
        auto *funcT = llvm::FunctionType::get(context.getBuilder().getInt32Ty(), false);
        auto *mainFunc = llvm::Function::Create(funcT, llvm::Function::ExternalLinkage, "main", *context.getModule());
        auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "entry", mainFunc);
        context.getBuilder().SetInsertPoint(block);

        //添加变量常量list
        context.log() << "Entering global const part" << std::endl;
        header->constList->codegen(context);
        context.log() << "Entering global type part" << std::endl;
        header->typeList->codegen(context);
        context.log() << "Entering global var part" << std::endl;
        header->varList->codegen(context);

        //添加subroutine
        context.is_subroutine = true;
        context.log() << "Entering global routine part" << std::endl;
        header->subroutineList->codegen(context);
        context.is_subroutine = false;

        context.getBuilder().SetInsertPoint(block);
        context.log() << "Entering global body part" << std::endl;
        body->codegen(context);
        context.getBuilder().CreateRet(context.getBuilder().getInt32(0));

        return nullptr;
    }

    llvm::Value *RoutineNode::codegen(CodegenContext &context)
    {
        context.log() << "Entering function " + name->name << std::endl;

        context.traces.push_back(name->name);

        std::vector<llvm::Type *> types;
        std::vector<std::string> names;

        //获得参数
        for (auto &p : params->getChildren()) 
        {
            auto *ty = p->type->findType(context);
            if (ty == nullptr)
                throw CodegenException("Unsupported function param type");
            types.push_back(ty);
            names.push_back(p->name->name);
            if (ty->isArrayTy())
            {
                if (p->type->type == Type::String)
                    context.setArrayEntry(name->name + "." + p->name->name, 0, 255);
                else if (p->type->type == Type::Array)
                {
                    auto arrTy = cast_node<ArrayTypeNode>(p->type);
                    assert(arrTy != nullptr);
                    context.setArrayEntry(name->name + "." + p->name->name, arrTy);
                    arrTy->insertNestedArray(name->name + "." + p->name->name, context);
                }
                else if (p->type->type == Type::Alias)
                {
                    std::string aliasName = cast_node<AliasTypeNode>(p->type)->name->name;
                    std::shared_ptr<ArrayTypeNode> a;
                    for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                        if ((a = context.getArrayAlias(*rit + "." + aliasName)) != nullptr)
                            break;
                    if (a == nullptr) a = context.getArrayAlias(aliasName);
                    assert(a != nullptr && "Fatal error: array type not found!");
                    context.setArrayEntry(name->name + "." + p->name->name, a);
                    a->insertNestedArray(name->name + "." + p->name->name, context);
                }
            }
            else if (ty->isStructTy())
            {
               
            }
        }

        //返回类型
        llvm::Type *retTy = this->retType->findType(context);
        if (retTy == nullptr) throw CodegenException("Unsupported function return type");
        if (retTy->isArrayTy())
        {
            if (!retTy->getArrayElementType()->isIntegerTy(8) || retTy->getArrayNumElements() != 256)
                throw CodegenException("Not support array as function return type");
            retTy = context.getBuilder().getInt8PtrTy();
            context.setArrayEntry(name->name + "." + name->name, 0, 255);
        }
        else if (retTy->isStructTy())
        {
            
        }

        //insert block
        auto *funcTy = llvm::FunctionType::get(retTy, types, false);
        auto *func = llvm::Function::Create(funcTy, llvm::Function::ExternalLinkage, name->name, *context.getModule());
        auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "entry", func);
        context.getBuilder().SetInsertPoint(block);

        auto index = 0;
        for (auto &arg : func->args())
        {
            auto *type = arg.getType();
            auto *local = context.getBuilder().CreateAlloca(type);
            context.setLocal(name->name + "." + names[index++], local);
            context.getBuilder().CreateStore(&arg, local);
        }

        context.log() << "Entering const part of function " << name->name << std::endl;
        header->constList->codegen(context);
        context.log() << "Entering type part of function " << name->name << std::endl;
        header->typeList->codegen(context);
        context.log() << "Entering var part of function " << name->name << std::endl;
        header->varList->codegen(context);

        context.log() << "Entering routine part of function " << name->name << std::endl;
        header->subroutineList->codegen(context);

        context.getBuilder().SetInsertPoint(block);
        if (retType->type != Type::Void)  // set the return variable
        {  
            auto *type = retType->findType(context);

            llvm::Value *local;
            if (type == nullptr) throw CodegenException("Unknown function return type");
            else if (type->isArrayTy())
            {
                if (type->getArrayElementType()->isIntegerTy(8) && type->getArrayNumElements() == 256) // String
                {
                    local = context.getBuilder().CreateAlloca(type);
                }
                else
                    throw CodegenException("Unknown function return type");
            }
            else
                local = context.getBuilder().CreateAlloca(type);
            assert(local != nullptr && "Fatal error: Local variable alloc failed!");
            context.setLocal(name->name + "." + name->name, local);
        }

        context.log() << "Entering body part of function " << name->name << std::endl;
        body->codegen(context);

        if (retType->type != Type::Void) 
        {
            auto *local = context.getLocal(name->name + "." + name->name);
            llvm::Value *ret = context.getBuilder().CreateLoad(local);
            if (ret->getType()->isArrayTy())
            {
                llvm::Value *tmpStr = context.getTempStrPtr();
                llvm::Value *zero = llvm::ConstantInt::get(context.getBuilder().getInt32Ty(), 0, false);
                llvm::Value *retPtr = context.getBuilder().CreateInBoundsGEP(local, {zero, zero});
                context.log() << "\tSysfunc STRCPY" << std::endl;
                context.getBuilder().CreateCall(context.strcpyFunc, {tmpStr, retPtr});
                context.log() << "\tSTRING return" << std::endl;
                context.getBuilder().CreateRet(tmpStr);
            }
            else
                context.getBuilder().CreateRet(ret);
        } 
        else 
        {
            context.getBuilder().CreateRetVoid();
        }

        context.traces.pop_back();  

        context.log() << "Leaving function " << name->name << std::endl;

        return nullptr;
    }

     llvm::Value *VarDeclNode::createGlobalArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
        // std::shared_ptr<ArrayTypeNode> arrTy = cast_node<ArrayTypeNode>(this->type);
        context.log() << "\tCreating array " << this->name->name << std::endl;
        auto *ty = arrTy->itemType->findType(context);
        llvm::Constant *z; // zero
        if (ty->isIntegerTy()) 
            z = llvm::ConstantInt::get(ty, 0);
        else if (ty->isDoubleTy())
            z = llvm::ConstantFP::get(ty, 0.0);
        else if (ty->isStructTy())
        {
           
        }
        else if (ty->isArrayTy())
        {
            z = llvm::Constant::getNullValue(ty);
            std::shared_ptr<ArrayTypeNode> arr;
            if (is_ptr_of<ArrayTypeNode>(arrTy->itemType))
                arr = cast_node<ArrayTypeNode>(arrTy->itemType);
            else if (is_ptr_of<StringTypeNode>(arrTy->itemType))
                arr = nullptr;
            else if (is_ptr_of<AliasTypeNode>(arrTy->itemType))
            {
                std::string aliasName = cast_node<AliasTypeNode>(arrTy->itemType)->name->name;
                for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                    if ((arr = context.getArrayAlias(*rit + "." + aliasName)) != nullptr)
                        break;
                if (arr == nullptr) arr = context.getArrayAlias(aliasName);
                if (arr == nullptr) assert(0 && "Fatal Error: Unexpected behavior!");
            }
            else assert(0 && "Fatal Error: Unexpected behavior!");
            if (arr == nullptr) // String
                context.setArrayEntry(this->name->name + "[]", 0, 255);
            else
            {
                context.setArrayEntry(this->name->name + "[]", arr);
                arr->insertNestedArray(this->name->name + "[]", context);
            }
        }
        else 
            throw CodegenException("Unsupported type of array");

        llvm::ConstantInt *startIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_start->codegen(context));
        if (!startIdx)
            throw CodegenException("Start index invalid");
        // else if (startIdx->getSExtValue() < 0)
        //     throw CodegenException("Start index must be greater than zero!");
        int start = startIdx->getSExtValue();
        
        int len = 0;
        llvm::ConstantInt *endIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_end->codegen(context));
        int end = endIdx->getSExtValue();
        if (!endIdx)
            throw CodegenException("End index invalid");
        else if (start > end)
            throw CodegenException("End index must be greater than start index!");
        else if (endIdx->getBitWidth() <= 32)
            len = end - start + 1;
        else
            throw CodegenException("End index overflow");

        context.log() << "\tArray info: start: " << start << " end: " << end << " len: " << len << std::endl;
        llvm::ArrayType* arr = llvm::ArrayType::get(ty, len);
        std::vector<llvm::Constant *> initVector;
        for (int i = 0; i < len; i++)
            initVector.push_back(z);
        auto *variable = llvm::ConstantArray::get(arr, initVector);

        llvm::Value *gv = new llvm::GlobalVariable(*context.getModule(), variable->getType(), false, llvm::GlobalVariable::ExternalLinkage, variable, this->name->name);
        context.log() << "\tCreated array " << this->name->name << std::endl;

        context.setArrayEntry(this->name->name, start, end);
        context.log() << "\tInserted to array table" << std::endl;

        return gv;
    }

    llvm::Value *VarDeclNode::createArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
        // std::shared_ptr<ArrayTypeNode> arrTy = cast_node<ArrayTypeNode>(this->type);
        context.log() << "\tCreating array " << this->name->name << std::endl;
        auto *ty = arrTy->itemType->findType(context);
        llvm::Constant *constant;
        if (ty->isIntegerTy()) 
            constant = llvm::ConstantInt::get(ty, 0);
        else if (ty->isDoubleTy())
            constant = llvm::ConstantFP::get(ty, 0.0);
        else if (ty->isStructTy())
        {
            
        }
        else if (ty->isArrayTy())
        {
            constant = nullptr;
            std::shared_ptr<ArrayTypeNode> arr;
            if (is_ptr_of<ArrayTypeNode>(arrTy->itemType))
                arr = cast_node<ArrayTypeNode>(arrTy->itemType);
            else if (is_ptr_of<StringTypeNode>(arrTy->itemType))
                arr = nullptr;
            else if (is_ptr_of<AliasTypeNode>(arrTy->itemType))
            {
                std::string aliasName = cast_node<AliasTypeNode>(arrTy->itemType)->name->name;
                for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                    if ((arr = context.getArrayAlias(*rit + "." + aliasName)) != nullptr)
                        break;
                if (arr == nullptr) arr = context.getArrayAlias(aliasName);
                if (arr == nullptr) assert(0 && "Fatal Error: Unexpected behavior!");
            }
            else assert(0 && "Fatal Error: Unexpected behavior!");
            if (arr == nullptr) // String
                context.setArrayEntry(this->name->name + "[]", 0, 255);
            else
            {
                context.setArrayEntry(this->name->name + "[]", arr);
                arr->insertNestedArray(this->name->name + "[]", context);
            }
        }
        else
            throw CodegenException("Unsupported type of array");

        llvm::ConstantInt *startIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_start->codegen(context));
        if (!startIdx)
            throw CodegenException("Start index invalid");
        // else if (startIdx->getSExtValue() < 0)
        //     throw CodegenException("Start index must be greater than zero!");
        int start = startIdx->getSExtValue();
        
        unsigned len = 0;
        llvm::ConstantInt *endIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_end->codegen(context));
        int end = endIdx->getSExtValue();
        if (!endIdx)
            throw CodegenException("End index invalid");
        else if (start > end)
            throw CodegenException("End index must be greater than start index!");
        else if (endIdx->getBitWidth() <= 32)
            len = end - start + 1;
        else
            throw CodegenException("End index overflow");
        
        context.log() << "\tArray info: start: " << start << " end: " << end << " len: " << len << std::endl;
        // llvm::ConstantInt *space = llvm::ConstantInt::get(context.getBuilder().getInt32Ty(), len);
        llvm::ArrayType *arrayTy = llvm::ArrayType::get(ty, len);
        // auto *local = context.getBuilder().CreateAlloca(ty, space);
        auto *local = context.getBuilder().CreateAlloca(arrayTy);
        auto success = context.setLocal(context.getTrace() + "." + this->name->name, local);
        if (!success) throw CodegenException("Duplicate identifier in var section of function " + context.getTrace() + ": " + this->name->name);
        context.log() << "\tCreated array " << this->name->name << std::endl;

        context.setArrayEntry(context.getTrace() + "." + this->name->name, start, end);
        context.log() << "\tInserted to array table" << std::endl;

        return local;
    }
    
    llvm::Value *VarDeclNode::codegen(CodegenContext &context)
    {
        if (context.is_subroutine)
        {
            if (type->type == Type::Alias)
            {
                std::string aliasName = cast_node<AliasTypeNode>(type)->name->name;
                context.log() << "\tSearching alias " << aliasName << std::endl;
                std::shared_ptr<ArrayTypeNode> arrTy = nullptr;
                for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                {
                    if ((arrTy = context.getArrayAlias(*rit + "." + aliasName)) != nullptr)
                        break;
                }
                if (arrTy == nullptr)
                    arrTy = context.getArrayAlias(aliasName);
                if (arrTy != nullptr)
                {
                    context.log() << "\tAlias is array" << std::endl;
                    return createArray(context, arrTy);
                }
                std::shared_ptr<RecordTypeNode> recTy = nullptr;
                for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                {
                    if ((recTy = context.getRecordAlias(*rit + "." + aliasName)) != nullptr)
                        break;
                }
                if (recTy == nullptr)
                    recTy = context.getRecordAlias(aliasName);

            }
            if (type->type == Type::Array)
                return createArray(context, cast_node<ArrayTypeNode>(this->type));
            else if (type->type == Type::String)
            {
                auto arrTy = make_new_node<ArrayTypeNode>(0, 255, Type::Char);
                return createArray(context, arrTy);
            }
            else
            {
                auto *local = context.getBuilder().CreateAlloca(type->findType(context));
                auto success = context.setLocal(context.getTrace() + "." + name->name, local);
                if (!success) throw CodegenException("Duplicate identifier in var section of function " + context.getTrace() + ": " + name->name);
                return local;
            }
        }
        else
        {
            if (context.getModule()->getGlobalVariable(name->name) != nullptr)
                throw CodegenException("Duplicate global variable: " + name->name);
            if (type->type == Type::Alias)
            {
                std::string aliasName = cast_node<AliasTypeNode>(type)->name->name;
                context.log() << "\tSearching alias " << aliasName << std::endl;
                std::shared_ptr<ArrayTypeNode> arrTy = context.getArrayAlias(aliasName);
                if (arrTy != nullptr)
                {
                    context.log() << "\tAlias is array" << std::endl;
                    return createGlobalArray(context, arrTy);
                }
                std::shared_ptr<RecordTypeNode> recTy = nullptr;
                for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                {
                    if ((recTy = context.getRecordAlias(*rit + "." + aliasName)) != nullptr)
                        break;
                }
                if (recTy == nullptr)
                    recTy = context.getRecordAlias(aliasName);
        
            }
            if (type->type == Type::Array)
                return createGlobalArray(context, cast_node<ArrayTypeNode>(this->type));
            else if (type->type == Type::String)
            {
                auto arrTy = make_new_node<ArrayTypeNode>(0, 255, Type::Char);
                return createGlobalArray(context, arrTy);
            }
            else
            {

                auto *ty = type->findType(context);
                llvm::Constant *constant;
                if (ty->isIntegerTy()) 
                    constant = llvm::ConstantInt::get(ty, 0);
                else if (ty->isDoubleTy())
                    constant = llvm::ConstantFP::get(ty, 0.0);
                else if (ty->isStructTy())
       	        {
                   
                }
                else
                    throw CodegenException("Unknown type");
                return new llvm::GlobalVariable(*context.getModule(), ty, false, llvm::GlobalVariable::ExternalLinkage, constant, name->name);
            }
        }
    }

    llvm::Value *ConstDeclNode::codegen(CodegenContext &context)
    {
        if (context.is_subroutine)
        {
            if (val->type == Type::String)
            {
                context.log() << "\tConst string declare" << std::endl;
                auto strVal = cast_node<StringNode>(val);
                auto *constant = llvm::ConstantDataArray::getString(llvm_context, strVal->val, true);
                bool success = context.setConst(context.getTrace() + "." + name->name, constant);
                if (!success) throw CodegenException("Duplicate identifier in const section of function " + context.getTrace() + ": " + name->name);
                context.log() << "\tAdded to symbol table" << std::endl;
                auto *gv = new llvm::GlobalVariable(*context.getModule(), constant->getType(), true, llvm::GlobalVariable::ExternalLinkage, constant, context.getTrace() + "." + name->name);
                context.log() << "\tCreated global variable" << std::endl;
                return gv;
            }
            else
            {
                context.log() << "\tConst declare" << std::endl;
                auto *constant = llvm::cast<llvm::Constant>(val->codegen(context));
                assert(constant != nullptr);
                bool success = context.setConst(context.getTrace() + "." + name->name, constant);
                success &= context.setConstVal(context.getTrace() + "." + name->name, constant);
                if (!success) throw CodegenException("Duplicate identifier in const section of function " + context.getTrace() + ": " + name->name);
                context.log() << "\tAdded to symbol table" << std::endl;
                return nullptr;
            } 
        }
        else
        {
            if (val->type == Type::String)
            {
                context.log() << "\tConst string declare" << std::endl;
                auto strVal = cast_node<StringNode>(val);
                auto *constant = llvm::ConstantDataArray::getString(llvm_context, strVal->val, true);
                context.setConst(name->name, constant);
                context.log() << "\tAdded to symbol table" << std::endl;
                auto *gv = new llvm::GlobalVariable(*context.getModule(), constant->getType(), true, llvm::GlobalVariable::ExternalLinkage, constant, name->name);
                context.log() << "\tCreated global variable" << std::endl;
                return gv;
            }
            else
            {
                context.log() << "\tConst declare" << std::endl;
                auto *constant = llvm::cast<llvm::Constant>(val->codegen(context));
                bool success = context.setConst(name->name, constant);
                success &= context.setConstVal(name->name, constant);
                if (!success) throw CodegenException("Duplicate identifier in const section of main program: " + name->name);
                context.log() << "\tAdded to symbol table" << std::endl;
                return nullptr;
            } 
        }
    }

    llvm::Value *TypeDeclNode::codegen(CodegenContext &context)
    {
        if (context.is_subroutine)
        {
            if (type->type == Type::Array)
            {
                bool success = context.setAlias(context.getTrace() + "." + name->name, type->findType(context));
                success &= context.setArrayAlias(context.getTrace() + "." + name->name, cast_node<ArrayTypeNode>(type));
                if (!success) throw CodegenException("Duplicate type alias in function " + context.getTrace() + ": " + name->name);
                context.log() << "\tArray alias in function " << context.getTrace() << ": " << name->name << std::endl;
            }
            else if (type->type == Type::Record)
            {
                bool success = context.setAlias(context.getTrace() + "." + name->name, type->findType(context));
                success &= context.setRecordAlias(context.getTrace() + "." + name->name, cast_node<RecordTypeNode>(type));
                if (!success) throw CodegenException("Duplicate type alias in function " + context.getTrace() + ": " + name->name);
                context.log() << "\tRecord alias in function " << context.getTrace() << ": " << name->name << std::endl;
            }
            else
            {
                bool success = context.setAlias(context.getTrace() + "." + name->name, type->findType(context));
                if (!success) throw CodegenException("Duplicate type alias in function " + context.getTrace() + ": " + name->name);
            }
        }
        else
        {
            if (type->type == Type::Array)
            {
                bool success = context.setAlias(name->name, type->findType(context));
                success &= context.setArrayAlias(name->name, cast_node<ArrayTypeNode>(type));
                if (!success) throw CodegenException("Duplicate type alias in main program: " + name->name);
                context.log() << "\tGlobal array alias: " << name->name << std::endl;
            }
            else if (type->type == Type::Record)
            {
                bool success = context.setAlias(name->name, type->findType(context));
                success &= context.setRecordAlias(name->name, cast_node<RecordTypeNode>(type));
                if (!success) throw CodegenException("Duplicate type alias in main program: " + name->name);
                context.log() << "\tGlobal record alias: " << name->name << std::endl;
            }
            else
            {
                bool success = context.setAlias(name->name, type->findType(context));
                if (!success) throw CodegenException("Duplicate type alias in main program: " + name->name);
            }        
        }
        return nullptr;
    }

     llvm::Value *BinaryExprNode::codegen(CodegenContext &context)
    {
        auto *lexp = lhs->codegen(context);
        auto *rexp = rhs->codegen(context);
        std::map<BinaryOp, llvm::CmpInst::Predicate> iCmpMap = {
                {BinaryOp::Gt, llvm::CmpInst::ICMP_SGT},
                {BinaryOp::Geq, llvm::CmpInst::ICMP_SGE},
                {BinaryOp::Lt, llvm::CmpInst::ICMP_SLT},
                {BinaryOp::Leq, llvm::CmpInst::ICMP_SLE},
                {BinaryOp::Eq, llvm::CmpInst::ICMP_EQ},
                {BinaryOp::Neq, llvm::CmpInst::ICMP_NE}
        };
        std::map<BinaryOp, llvm::CmpInst::Predicate> fCmpMap = {
                {BinaryOp::Gt, llvm::CmpInst::FCMP_OGT},
                {BinaryOp::Geq, llvm::CmpInst::FCMP_OGE},
                {BinaryOp::Lt, llvm::CmpInst::FCMP_OLT},
                {BinaryOp::Leq, llvm::CmpInst::FCMP_OLE},
                {BinaryOp::Eq, llvm::CmpInst::FCMP_OEQ},
                {BinaryOp::Neq, llvm::CmpInst::FCMP_ONE}
        };

        if (lexp->getType()->isDoubleTy() || rexp->getType()->isDoubleTy()) 
        {
            if (!lexp->getType()->isDoubleTy()) 
            {
                lexp = context.getBuilder().CreateSIToFP(lexp, context.getBuilder().getDoubleTy());
            }
            else if (!rexp->getType()->isDoubleTy()) 
            {
                rexp = context.getBuilder().CreateSIToFP(rexp, context.getBuilder().getDoubleTy());
            }
            auto it = fCmpMap.find(op);
            if (it != fCmpMap.end())
                return context.getBuilder().CreateFCmp(it->second, lexp, rexp);
            llvm::Instruction::BinaryOps binop;
            switch(op) 
            {
                case BinaryOp::Plus: binop = llvm::Instruction::FAdd; break;
                case BinaryOp::Minus: binop = llvm::Instruction::FSub; break;
                case BinaryOp::Truediv: binop = llvm::Instruction::FDiv; break;
                case BinaryOp::Mul: binop = llvm::Instruction::FMul; break;
                default: throw CodegenException("Invaild operator for REAL type");
            }
            return context.getBuilder().CreateBinOp(binop, lexp, rexp);
        }
        else if (lexp->getType()->isIntegerTy(32) && rexp->getType()->isIntegerTy(32)) 
        {
            auto it = iCmpMap.find(op);
            if (it != iCmpMap.end())
                return context.getBuilder().CreateICmp(it->second, lexp, rexp);
            llvm::Instruction::BinaryOps binop;
            switch(op) 
            {
                case BinaryOp::Plus: binop = llvm::Instruction::Add; break;
                case BinaryOp::Minus: binop = llvm::Instruction::Sub; break;
                case BinaryOp::Div: binop = llvm::Instruction::SDiv; break;
                case BinaryOp::Mod: binop = llvm::Instruction::SRem; break;
                case BinaryOp::Mul: binop = llvm::Instruction::Mul; break;
                case BinaryOp::And: binop = llvm::Instruction::And; break;
                case BinaryOp::Or: binop = llvm::Instruction::Or; break;
                case BinaryOp::Xor: binop = llvm::Instruction::Xor; break;
                case BinaryOp::Truediv:
                    lexp = context.getBuilder().CreateSIToFP(lexp, context.getBuilder().getDoubleTy());
                    rexp = context.getBuilder().CreateSIToFP(rexp, context.getBuilder().getDoubleTy());
                    binop = llvm::Instruction::FDiv; 
                    break;
                default: throw CodegenException("Invaild operator for INTEGER type");
            }
            return context.getBuilder().CreateBinOp(binop, lexp, rexp);
        }
        else if (lexp->getType()->isIntegerTy(1) && rexp->getType()->isIntegerTy(1)) 
        {
            auto it = iCmpMap.find(op);
            if (it != iCmpMap.end())
                return context.getBuilder().CreateICmp(it->second, lexp, rexp);
            llvm::Instruction::BinaryOps binop;
            switch(op) 
            {
                case BinaryOp::And: binop = llvm::Instruction::And; break;
                case BinaryOp::Or: binop = llvm::Instruction::Or; break;
                case BinaryOp::Xor: binop = llvm::Instruction::Xor; break;
                default: throw CodegenException("Invaild operator for BOOLEAN type");
            }
            return context.getBuilder().CreateBinOp(binop, lexp, rexp);
        }
        else if (lexp->getType()->isIntegerTy(8) && rexp->getType()->isIntegerTy(8)) 
        {
            auto it = iCmpMap.find(op);
            if (it != iCmpMap.end())
                return context.getBuilder().CreateICmp(it->second, lexp, rexp);
            else
                throw CodegenException("Invaild operator for CHAR type");
        }
        else
            throw CodegenException("Invaild operation between different types");
    }

    llvm::Value *CustomProcNode::codegen(CodegenContext &context)
    {
        auto *func = context.getModule()->getFunction(name->name);
        if (!func)
            throw CodegenException("Function not found: " + name->name + "()");
        size_t argCnt = 0;
        int index = 0;
        if (args != nullptr)
            argCnt = args->getChildren().size();
        if (func->arg_size() != argCnt)
            throw CodegenException("Wrong number of arguments: " + name->name + "()");
        auto *funcTy = func->getFunctionType();
        std::vector<llvm::Value*> values;
        if (args != nullptr)
            for (auto &arg : args->getChildren())
            {
                llvm::Value *argVal = arg->codegen(context);
                auto *paramTy = funcTy->getParamType(index), *argTy = argVal->getType();
                if (paramTy->isDoubleTy() && argTy->isIntegerTy(32))
                    argVal = context.getBuilder().CreateSIToFP(argVal, paramTy);
                else if (argTy->isDoubleTy() && paramTy->isIntegerTy(32))
                {
                    std::cerr << "Warning: casting REAL type to INTEGER type when calling function " << name->name << "()" << std::endl;
                    argVal = context.getBuilder().CreateFPToSI(argVal, paramTy);
                }
                else if (funcTy->getParamType(index) != argVal->getType())
                    throw CodegenException("Incompatible type in the " + std::to_string(index) + "th arg when calling " + name->name + "()");
                values.push_back(argVal);
                index++;
            }
     
        return context.getBuilder().CreateCall(func, values);
    }

    llvm::Value *SysProcNode::codegen(CodegenContext &context)
    {
        if (name == SysFunc::Write || name == SysFunc::Writeln) {
            context.log() << "\tSysfunc WRITE" << std::endl;
            if (this->args != nullptr)
                for (auto &arg : this->args->getChildren()) {
                    auto *value = arg->codegen(context);
                    assert(value != nullptr);
                    auto x = value->getType();
                    std::vector<llvm::Value*> func_args;
                    if (value->getType()->isIntegerTy(32)) 
                    {
                        std::string setWid = "%";
                        setWid.append(width);
                        setWid.append("d");
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr(setWid));
                        func_args.push_back(value);
                    }
                    else if (value->getType()->isIntegerTy(8)) 
                    {
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%c")); 
                        func_args.push_back(value); 
                    }
                    else if (value->getType()->isDoubleTy()) 
                    {
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%f"));
                        func_args.push_back(value);
                    }
                    else if (value->getType()->isArrayTy()) // String
                    {
                        auto *a = llvm::cast<llvm::ArrayType>(x);
                        if (!a->getElementType()->isIntegerTy(8))
                            throw CodegenException("Cannot print a non-char array");
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%s"));
                        llvm::Value *valuePtr;
                        if (is_ptr_of<LeftExprNode>(arg))
                        {
                            auto argId = cast_node<LeftExprNode>(arg);
                            valuePtr = argId->getPtr(context);
                        }
                        else if (is_ptr_of<CustomProcNode>(arg))
                            valuePtr = value;
                        else
                            assert(0);
                        func_args.push_back(valuePtr);
                    }
                    else if (value->getType()->isPointerTy()) // String
                    {
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%s"));
                        func_args.push_back(value);
                    }
                    else 
                        throw CodegenException("Incompatible type in read(): expected char, integer, real, array, string");
                    context.getBuilder().CreateCall(context.printfFunc, func_args);
                }
            if (name == SysFunc::Writeln) {
                context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr("\n"));
            }
            return nullptr;
        }
        else if (name == SysFunc::Read || name == SysFunc::Readln)
        {
            context.log() << "\tSysfunc READ" << std::endl;
            if (this->args != nullptr)
                for (auto &arg : args->getChildren())
                {
                    llvm::Value *ptr;
                    if (is_ptr_of<LeftExprNode>(arg))
                        ptr = cast_node<LeftExprNode>(arg)->getPtr(context);
                    else
                        throw CodegenException("Argument in read() must be identifier or array/record reference");
                    std::vector<llvm::Value*> func_args;
                    if (ptr->getType()->getPointerElementType()->isIntegerTy(8))
                    { 
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%c")); 
                        func_args.push_back(ptr); 
                    }
                    else if (ptr->getType()->getPointerElementType()->isIntegerTy(32))
                    { 
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%d")); 
                        func_args.push_back(ptr); 
                    }
                    else if (ptr->getType()->getPointerElementType()->isDoubleTy())
                    { 
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%lf")); 
                        func_args.push_back(ptr); 
                    }
                    // String support
                    else if (ptr->getType()->getPointerElementType()->isArrayTy() && llvm::cast<llvm::ArrayType>(ptr->getType()->getPointerElementType())->getElementType()->isIntegerTy(8))
                    {
                        if (name == SysFunc::Read)
                            func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%s"));
                        else // Readln
                        {
                            func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%[^\n]"));
                            if (arg != this->args->getChildren().back())
                                std::cerr << "Warning in readln(): string type should be the last argument in readln(), otherwise the subsequent arguments cannot be read!" << std::endl;
                        }
                        func_args.push_back(ptr);
                    }
                    else
                        throw CodegenException("Incompatible type in read(): expected char, integer, real, string");
                    context.getBuilder().CreateCall(context.scanfFunc, func_args);
                }
            if (name == SysFunc::Readln)
            {
                // Flush all other inputs in this line
                context.getBuilder().CreateCall(context.scanfFunc, context.getBuilder().CreateGlobalStringPtr("%*[^\n]"));
                // Flush the final '\n'
                context.getBuilder().CreateCall(context.getcharFunc);
            }
            return nullptr;
        }
        else if (name == SysFunc::Concat)
        {
            context.log() << "\tSysfunc CONCAT" << std::endl;
            std::string format;
            std::list<llvm::Value*> func_args;
            for (auto &arg : this->args->getChildren()) {
                auto *value = arg->codegen(context);
                auto x = value->getType();
                if (value->getType()->isIntegerTy(32)) 
                {
                    format += "%d";
                    func_args.push_back(value);
                }
                else if (value->getType()->isIntegerTy(8)) 
                {
                    format += "%c";
                    func_args.push_back(value);
                }
                else if (value->getType()->isDoubleTy()) 
                {
                    format += "%f";
                    func_args.push_back(value);
                }
                else if (value->getType()->isArrayTy()) // String
                {
                    auto *a = llvm::cast<llvm::ArrayType>(x);
                    if (!a->getElementType()->isIntegerTy(8))
                        throw CodegenException("Cannot concat a non-char array");
                    format += "%s";
                    llvm::Value *valuePtr;
                    if (is_ptr_of<LeftExprNode>(arg))
                    {
                        auto argId = cast_node<LeftExprNode>(arg);
                        valuePtr = argId->getPtr(context);
                    }
                    else
                        assert(0);
                    func_args.push_back(valuePtr);
                }
                else if (value->getType()->isPointerTy()) // String
                {
                    format += "%s";
                    func_args.push_back(value);
                }
                else 
                    throw CodegenException("Incompatible type in concat(): expected char, integer, real, array, string");        
            }
            func_args.push_front(context.getBuilder().CreateGlobalStringPtr(format.c_str()));
            func_args.push_front(context.getTempStrPtr());
            // sprintf(__tmp_str, "...formats", ...args);
            std::vector<llvm::Value*> func_args_vec(func_args.begin(), func_args.end());
            context.getBuilder().CreateCall(context.sprintfFunc, func_args_vec);
            return context.getTempStrPtr();
        }
        else if (name == SysFunc::Length)
        {
            context.log() << "\tSysfunc LENGTH" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in length(): expected 1");
            auto arg = args->getChildren().front();
            auto *value = arg->codegen(context);
            auto *ty = value->getType();
            llvm::Value *zero = llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), 0);
            // context.log() << ty->getTypeID() << std::endl;
            if (ty->isArrayTy() && ty->getArrayElementType()->isIntegerTy(8))
            {
                llvm::Value *valPtr;
                if (is_ptr_of<LeftExprNode>(arg))
                    valPtr = context.getBuilder().CreateInBoundsGEP(cast_node<LeftExprNode>(arg)->getPtr(context), {zero, zero});
                else if (is_ptr_of<CustomProcNode>(arg))
                    valPtr = context.getBuilder().CreateInBoundsGEP(value, {zero, zero});
                else
                    assert(0);
                return context.getBuilder().CreateCall(context.strlenFunc, valPtr);
            }
            else if (ty->isPointerTy())
            {
               
                if(ty == context.getBuilder().getInt8PtrTy())
                    return context.getBuilder().CreateCall(context.strlenFunc, value);
                else if (ty->getPointerElementType()->isIntegerTy(8))
                {
                    llvm::Value *valPtr = context.getBuilder().CreateGEP(value, zero);
                    return context.getBuilder().CreateCall(context.atoiFunc, valPtr);
                }
                else if (ty->getPointerElementType()->isArrayTy())
                {
                    llvm::Value *valPtr = context.getBuilder().CreateInBoundsGEP(value, {zero, zero});
                    return context.getBuilder().CreateCall(context.strlenFunc, valPtr);
                }
                else
                {
                    throw CodegenException("Incompatible type in length(): expected string");
                }
            }
            else
            {
                throw CodegenException("Incompatible type in length(): expected string");
            }
        }
        else if (name == SysFunc::Abs)
        {
            context.log() << "\tSysfunc ABS" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in abs(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))
                return context.getBuilder().CreateCall(context.absFunc, value);
            else if (value->getType()->isDoubleTy())
                return context.getBuilder().CreateCall(context.fabsFunc, value);
            else
                throw CodegenException("Incompatible type in abs(): expected integer, real");
        }
        else if (name == SysFunc::Val)
        {
            context.log() << "\tSysfunc VAL" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in val(): expected 1");
            auto arg = args->getChildren().front();
            auto *value = arg->codegen(context);
            auto *ty = value->getType();
            llvm::Value *zero = llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), 0);
            if (ty->isArrayTy() && ty->getArrayElementType()->isIntegerTy(8))
            {
                llvm::Value *valPtr;
                if (is_ptr_of<LeftExprNode>(arg))
                    valPtr = context.getBuilder().CreateInBoundsGEP(cast_node<LeftExprNode>(arg)->getPtr(context), {zero, zero});
               
                else if (is_ptr_of<CustomProcNode>(arg))
                    valPtr = context.getBuilder().CreateInBoundsGEP(value, {zero, zero});
                else
                    assert(0);
                return context.getBuilder().CreateCall(context.atoiFunc, valPtr);
            }
            else if (ty->isPointerTy())
            {
                if(ty == context.getBuilder().getInt8PtrTy())
                    return context.getBuilder().CreateCall(context.atoiFunc, value);
                else if (ty->getPointerElementType()->isIntegerTy(8))
                {
                    llvm::Value *valPtr = context.getBuilder().CreateGEP(value, zero);
                    return context.getBuilder().CreateCall(context.atoiFunc, valPtr);
                }
                else if (ty->getPointerElementType()->isArrayTy())
                {
                    llvm::Value *valPtr = context.getBuilder().CreateInBoundsGEP(value, {zero, zero});
                    return context.getBuilder().CreateCall(context.atoiFunc, valPtr);
                }
                else
                    throw CodegenException("Incompatible type in val(): expected string");
            }
            else
                throw CodegenException("Incompatible type in val(): expected string");
        }
        else if (name == SysFunc::Str)
        {
            context.log() << "\tSysfunc STR" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in str(): expected 1");
            auto arg = args->getChildren().front();
            auto *value = arg->codegen(context);
            auto *ty = value->getType();
            llvm::Value *zero = llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), 0);
            if (ty->isIntegerTy(8))
            {
                context.getBuilder().CreateCall(context.sprintfFunc, {context.getTempStrPtr(), context.getBuilder().CreateGlobalStringPtr("%c"), value});
                return context.getTempStrPtr();
            }
            else if (ty->isIntegerTy(32))
            {
                context.getBuilder().CreateCall(context.sprintfFunc, {context.getTempStrPtr(), context.getBuilder().CreateGlobalStringPtr("%d"), value});
                return context.getTempStrPtr();
            }
            else if (ty->isDoubleTy())
            {
                context.getBuilder().CreateCall(context.sprintfFunc, {context.getTempStrPtr(), context.getBuilder().CreateGlobalStringPtr("%f"), value});
                return context.getTempStrPtr();
            }
            else
                throw CodegenException("Incompatible type in str(): expected integer, char, real");
        }
        else if (name == SysFunc::Abs)
        {
            context.log() << "\tSysfunc ABS" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in abs(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))
                return context.getBuilder().CreateCall(context.absFunc, value);
            else if (value->getType()->isDoubleTy())
                return context.getBuilder().CreateCall(context.fabsFunc, value);
            else
                throw CodegenException("Incompatible type in abs(): expected integer, real");
        }
        else if (name == SysFunc::Sqrt)
        {
            context.log() << "\tSysfunc SQRT" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments in sqrt(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            auto *double_ty = context.getBuilder().getDoubleTy();
            if (value->getType()->isIntegerTy(32))
                value = context.getBuilder().CreateSIToFP(value, double_ty);
            else if (!value->getType()->isDoubleTy())
                throw CodegenException("Incompatible type in sqrt(): expected integer, real");
            return context.getBuilder().CreateCall(context.sqrtFunc, value);
        }
        else if (name == SysFunc::Sqr)
        {
            context.log() << "\tSysfunc SQR" << std::endl;
            if (args->getChildren().size() != 1)
                throw CodegenException("Wrong number of arguments: sqr()");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))      
                return context.getBuilder().CreateBinOp(llvm::Instruction::Mul, value, value);
            else if (value->getType()->isDoubleTy())    
                return context.getBuilder().CreateBinOp(llvm::Instruction::FMul, value, value);
            else
                throw CodegenException("Incompatible type in sqr(): expected char");
        }
        else if (name == SysFunc::Chr)
        {
            
        }
        else if (name == SysFunc::Ord)
        {
        
        }
        else if (name == SysFunc::Pred)
        {
            
        }
        else if (name == SysFunc::Succ)
        {
            
        }
    }
