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

        auto *funcT = llvm::FunctionType::get(context.getBuilder().getInt32Ty(), false);
        auto *mainFunc = llvm::Function::Create(funcT, llvm::Function::ExternalLinkage, "main", *context.getModule());
        auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "entry", mainFunc);
        context.getBuilder().SetInsertPoint(block);

        
        if(init_part->content !="")init_part->codegen(context);

        //添加变量常量list
        routine_head->constList->codegen(context);
        routine_head->typeList->codegen(context);
        routine_head->varList->codegen(context);

        //添加subroutine
        context.is_subroutine = true;
        routine_head->subroutineList->codegen(context);
        context.is_subroutine = false;

        context.getBuilder().SetInsertPoint(block);
        routine_body->codegen(context);
        context.getBuilder().CreateRet(context.getBuilder().getInt32(0));

        return nullptr;
    }

    llvm::Value *InitNode::codegen(CodegenContext &context){

        context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr(content));
        context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr("\n"));
        return nullptr;

    }

    llvm::Value *RoutineNode::codegen(CodegenContext &context)
    {
        context.traces.push_back(name->name);

        std::vector<llvm::Type *> types;
        std::vector<std::string> names;

        //获得参数
        for (auto &p : params->getChildren()) 
        {
            auto *ty = p->type->findType(context);
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
        if (retTy->isArrayTy())
        {
  
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

    
        routine_head->constList->codegen(context);
        routine_head->typeList->codegen(context);
        routine_head->varList->codegen(context);
        routine_head->subroutineList->codegen(context);

        context.getBuilder().SetInsertPoint(block);
        if (retType->type != Type::Void)  // set the return variable
        {  
            auto *type = retType->findType(context);

            llvm::Value *local;
            if (type->isArrayTy())
            {
                if (type->getArrayElementType()->isIntegerTy(8) && type->getArrayNumElements() == 256) // String
                {
                    local = context.getBuilder().CreateAlloca(type);
                }
                else
                    throw CodegenExcep("Unknown function return type");
            }
            else
                local = context.getBuilder().CreateAlloca(type);
            assert(local != nullptr && "Fatal error: Local variable alloc failed!");
            context.setLocal(name->name + "." + name->name, local);
        }

        routine_body->codegen(context);

        if (retType->type != Type::Void) 
        {
            auto *local = context.getLocal(name->name + "." + name->name);
            llvm::Value *ret = context.getBuilder().CreateLoad(local);
            if (ret->getType()->isArrayTy())
            {
                llvm::Value *tmpStr = context.getTempStrPtr();
                llvm::Value *zero = llvm::ConstantInt::get(context.getBuilder().getInt32Ty(), 0, false);
                llvm::Value *retPtr = context.getBuilder().CreateInBoundsGEP(local, {zero, zero});
                context.getBuilder().CreateCall(context.strcpyFunc, {tmpStr, retPtr});
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

        return nullptr;
    }

     llvm::Value *VarDeclNode::createGlobalArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
       
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
            throw CodegenExcep("Unsupported type of array");

        llvm::ConstantInt *startIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_start->codegen(context));
        int start = startIdx->getSExtValue();
        
        int len = 0;
        llvm::ConstantInt *endIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_end->codegen(context));
        int end = endIdx->getSExtValue();
        if (endIdx->getBitWidth() <= 32)
            len = end - start + 1;
        else
            throw CodegenExcep("End index overflow");

        llvm::ArrayType* arr = llvm::ArrayType::get(ty, len);
        std::vector<llvm::Constant *> initVector;
        for (int i = 0; i < len; i++)
            initVector.push_back(z);
        auto *variable = llvm::ConstantArray::get(arr, initVector);

        llvm::Value *gv = new llvm::GlobalVariable(*context.getModule(), variable->getType(), false, llvm::GlobalVariable::ExternalLinkage, variable, this->name->name);
        context.setArrayEntry(this->name->name, start, end);

        return gv;
    }

    llvm::Value *VarDeclNode::createArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
    
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
            throw CodegenExcep("Unsupported type of array");

        llvm::ConstantInt *startIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_start->codegen(context));
       
        int start = startIdx->getSExtValue();
        
        unsigned len = 0;
        llvm::ConstantInt *endIdx = llvm::dyn_cast<llvm::ConstantInt>(arrTy->range_end->codegen(context));
        int end = endIdx->getSExtValue();
        if (endIdx->getBitWidth() <= 32)
            len = end - start + 1;
        else
            throw CodegenExcep("End index overflow");
        
        llvm::ArrayType *arrayTy = llvm::ArrayType::get(ty, len);
        auto *local = context.getBuilder().CreateAlloca(arrayTy);
        auto success = context.setLocal(context.getTrace() + "." + this->name->name, local);
        if (!success) throw CodegenExcep("Duplicate identifier in var section of function " + context.getTrace() + ": " + this->name->name);
        context.setArrayEntry(context.getTrace() + "." + this->name->name, start, end);

        return local;
    }
    
    llvm::Value *VarDeclNode::codegen(CodegenContext &context)
    {
        if (context.is_subroutine)
        {
            if (type->type == Type::Alias)
            {
                std::string aliasName = cast_node<AliasTypeNode>(type)->name->name;
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
                if (!success) throw CodegenExcep("Duplicate identifier in var section of function " + context.getTrace() + ": " + name->name);
                return local;
            }
        }
        else
        {
            if (type->type == Type::Alias)
            {
                std::string aliasName = cast_node<AliasTypeNode>(type)->name->name;
                std::shared_ptr<ArrayTypeNode> arrTy = context.getArrayAlias(aliasName);
                if (arrTy != nullptr)
                {
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
                auto strVal = cast_node<StringNode>(val);
                auto *constant = llvm::ConstantDataArray::getString(llvm_context, strVal->val, true);
                bool success = context.setConst(context.getTrace() + "." + name->name, constant);
                auto *gv = new llvm::GlobalVariable(*context.getModule(), constant->getType(), true, llvm::GlobalVariable::ExternalLinkage, constant, context.getTrace() + "." + name->name);
                return gv;
            }
            else
            {
               
                auto *constant = llvm::cast<llvm::Constant>(val->codegen(context));
                assert(constant != nullptr);
                bool success = context.setConst(context.getTrace() + "." + name->name, constant);
                success &= context.setConstVal(context.getTrace() + "." + name->name, constant);
                return nullptr;
            } 
        }
        else
        {
            if (val->type == Type::String)
            {
                auto strVal = cast_node<StringNode>(val);
                auto *constant = llvm::ConstantDataArray::getString(llvm_context, strVal->val, true);
                context.setConst(name->name, constant);
                auto *gv = new llvm::GlobalVariable(*context.getModule(), constant->getType(), true, llvm::GlobalVariable::ExternalLinkage, constant, name->name);
                return gv;
            }
            else
            { 
                auto *constant = llvm::cast<llvm::Constant>(val->codegen(context));
                bool success = context.setConst(name->name, constant);
                success &= context.setConstVal(name->name, constant);
               
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
            }
            else if (type->type == Type::Record)
            {
                bool success = context.setAlias(context.getTrace() + "." + name->name, type->findType(context));
                success &= context.setRecordAlias(context.getTrace() + "." + name->name, cast_node<RecordTypeNode>(type));
            }
            else
            {
                bool success = context.setAlias(context.getTrace() + "." + name->name, type->findType(context));
            }
        }
        else
        {
            if (type->type == Type::Array)
            {
                bool success = context.setAlias(name->name, type->findType(context));
                success &= context.setArrayAlias(name->name, cast_node<ArrayTypeNode>(type));
            
            }
            else if (type->type == Type::Record)
            {
                bool success = context.setAlias(name->name, type->findType(context));
                success &= context.setRecordAlias(name->name, cast_node<RecordTypeNode>(type));
            
            }
            else
            {
                bool success = context.setAlias(name->name, type->findType(context));
            
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
                default: throw CodegenExcep("Invaild operator for REAL type");
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
                default: throw CodegenExcep("Invaild operator for INTEGER type");
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
                default: throw CodegenExcep("Invaild operator for BOOLEAN type");
            }
            return context.getBuilder().CreateBinOp(binop, lexp, rexp);
        }
        else if (lexp->getType()->isIntegerTy(8) && rexp->getType()->isIntegerTy(8)) 
        {
            auto it = iCmpMap.find(op);
            if (it != iCmpMap.end())
                return context.getBuilder().CreateICmp(it->second, lexp, rexp);
            else
                throw CodegenExcep("Invaild operator for CHAR type");
        }
        else
            throw CodegenExcep("Invaild operation between different types");
    }

    llvm::Value *CustomProcNode::codegen(CodegenContext &context)
    {
        auto *func = context.getModule()->getFunction(name->name);
        if (!func)
            throw CodegenExcep("Function not found: " + name->name + "()");
        size_t argCnt = 0;
        int index = 0;
        if (args != nullptr)
            argCnt = args->getChildren().size();
        if (func->arg_size() != argCnt)
            throw CodegenExcep("Wrong number of arguments: " + name->name + "()");
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
                    throw CodegenExcep("Incompatible type in the " + std::to_string(index) + "th arg when calling " + name->name + "()");
                values.push_back(argVal);
                index++;
            }
     
        return context.getBuilder().CreateCall(func, values);
    }

    llvm::Value *SysProcNode::codegen(CodegenContext &context)
    {
        if (name == SysFunc::Write || name == SysFunc::Writeln) {
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
                            throw CodegenExcep("Cannot print a non-char array");
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
                        throw CodegenExcep("Incompatible type in read(): expected char, integer, real, array, string");
                    context.getBuilder().CreateCall(context.printfFunc, func_args);
                }
            if (name == SysFunc::Writeln) {
                context.getBuilder().CreateCall(context.printfFunc, context.getBuilder().CreateGlobalStringPtr("\n"));
            }
            return nullptr;
        }
        else if (name == SysFunc::Read || name == SysFunc::Readln)
        {
            if (this->args != nullptr)
                for (auto &arg : args->getChildren())
                {
                    llvm::Value *ptr;
                    if (is_ptr_of<LeftExprNode>(arg))
                        ptr = cast_node<LeftExprNode>(arg)->getPtr(context);
                    else
                        throw CodegenExcep("Argument in read() must be identifier or array/record reference");
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
                        throw CodegenExcep("Incompatible type in read(): expected char, integer, real, string");
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
                        throw CodegenExcep("Cannot concat a non-char array");
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
                    throw CodegenExcep("Incompatible type in concat(): expected char, integer, real, array, string");        
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
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in length(): expected 1");
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
                    throw CodegenExcep("Incompatible type in length(): expected string");
                }
            }
            else
            {
                throw CodegenExcep("Incompatible type in length(): expected string");
            }
        }
        else if (name == SysFunc::Abs)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in abs(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))
                return context.getBuilder().CreateCall(context.absFunc, value);
            else if (value->getType()->isDoubleTy())
                return context.getBuilder().CreateCall(context.fabsFunc, value);
            else
                throw CodegenExcep("Incompatible type in abs(): expected integer, real");
        }
        else if (name == SysFunc::Val)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in val(): expected 1");
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
                    throw CodegenExcep("Incompatible type in val(): expected string");
            }
            else
                throw CodegenExcep("Incompatible type in val(): expected string");
        }
        else if (name == SysFunc::Str)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in str(): expected 1");
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
                throw CodegenExcep("Incompatible type in str(): expected integer, char, real");
        }
        else if (name == SysFunc::Abs)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in abs(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))
                return context.getBuilder().CreateCall(context.absFunc, value);
            else if (value->getType()->isDoubleTy())
                return context.getBuilder().CreateCall(context.fabsFunc, value);
            else
                throw CodegenExcep("Incompatible type in abs(): expected integer, real");
        }
        else if (name == SysFunc::Sqrt)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments in sqrt(): expected 1");
            auto *value = args->getChildren().front()->codegen(context);
            auto *double_ty = context.getBuilder().getDoubleTy();
            if (value->getType()->isIntegerTy(32))
                value = context.getBuilder().CreateSIToFP(value, double_ty);
            else if (!value->getType()->isDoubleTy())
                throw CodegenExcep("Incompatible type in sqrt(): expected integer, real");
            return context.getBuilder().CreateCall(context.sqrtFunc, value);
        }
        else if (name == SysFunc::Sqr)
        {
            if (args->getChildren().size() != 1)
                throw CodegenExcep("Wrong number of arguments: sqr()");
            auto *value = args->getChildren().front()->codegen(context);
            if (value->getType()->isIntegerTy(32))      
                return context.getBuilder().CreateBinOp(llvm::Instruction::Mul, value, value);
            else if (value->getType()->isDoubleTy())    
                return context.getBuilder().CreateBinOp(llvm::Instruction::FMul, value, value);
            else
                throw CodegenExcep("Incompatible type in sqr(): expected char");
        }
        
    }

    llvm::Value *RecordRefNode::codegen(CodegenContext &context)
    {
        auto *ptr = this->getPtr(context);
        return context.getBuilder().CreateLoad(ptr);
    }
    llvm::Value *RecordRefNode::getPtr(CodegenContext &context)
    {
        llvm::Value *value = name->getPtr(context);
        assert(value != nullptr);
        std::shared_ptr<RecordTypeNode> recTy = nullptr;
        for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
        {
            if ((recTy = context.getRecordAlias(*rit + "." + name->getSymbolName())) != nullptr)
                break;
        }
        if (recTy == nullptr) recTy = context.getRecordAlias(name->getSymbolName());
        if (recTy == nullptr) throw CodegenExcep(name->getSymbolName() + " is not a record");
        assert(value->getType()->getPointerElementType()->isStructTy());
	    llvm::Value *idx = recTy->getFieldIdx(field->name, context);
        if (idx == nullptr)
            throw CodegenExcep("'" + field->name + "' is not in record field of " + name->getSymbolName());
        llvm::Value *zero = llvm::ConstantInt::get(context.getBuilder().getInt32Ty(), 0, false);
        return context.getBuilder().CreateInBoundsGEP(value, {zero, idx});
    }
    llvm::Value *RecordRefNode::getAssignPtr(CodegenContext &context)
    {
        return this->getPtr(context);
    }
    const std::string RecordRefNode::getSymbolName()
    {
        return this->name->getSymbolName() + "." + this->field->name;
    }

    llvm::Value *ArrayRefNode::codegen(CodegenContext &context) 
    {
        return context.getBuilder().CreateLoad(this->getPtr(context));
    }

    llvm::Value *ArrayRefNode::getAssignPtr(CodegenContext &context) 
    {
        llvm::Value *value = arr->getAssignPtr(context);
        assert(value != nullptr);
        return this->getPtr(context);
    }
    const std::string ArrayRefNode::getSymbolName()
    {
        return this->arr->getSymbolName() + "[]";
    }

    llvm::Value *ArrayRefNode::getPtr(CodegenContext &context) 
    {
        llvm::Value *value = arr->getPtr(context);
        assert(value != nullptr);
        auto *idx_value = context.getBuilder().CreateIntCast(this->index->codegen(context), context.getBuilder().getInt32Ty(), true);
        auto *ptr_type = value->getType()->getPointerElementType();
        std::vector<llvm::Value*> idx;

        idx.push_back(llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), 0));
        std::shared_ptr<std::pair<int,int>> range;
        // bool is_local = false;
        for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
        {
            if ((range = context.getArrayEntry(*rit + "." + arr->getSymbolName())) != nullptr)
            {
                // is_local = true;
                break;
            }
        }
        if (range == nullptr)
            range = context.getArrayEntry(arr->getSymbolName());
        if (ptr_type->isArrayTy())
        {
            if (range == nullptr) std::cout << arr->getSymbolName() << std::endl;
            assert(range != nullptr && "Fatal error: Array not found in array table!");
        }
        else if (range == nullptr)
            throw CodegenExcep(arr->getSymbolName() + " is not an array");
        
        llvm::ConstantInt *const_idx = llvm::dyn_cast<llvm::ConstantInt>(idx_value);
        if (const_idx != nullptr)
        {
            int int_idx = const_idx->getSExtValue();
            if (int_idx < range->first || int_idx > range->second)
                std::cerr << "Warning: index out of bound when visiting array '" + arr->getSymbolName() + "'" << std::endl;
        }
        if (range->first != 0)
        {
            llvm::Value *range_start = llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), range->first);
            llvm::Value *trueIdx = context.getBuilder().CreateBinOp(llvm::Instruction::Sub, idx_value, range_start);
            if (ptr_type->isArrayTy())
                idx.push_back(trueIdx);
            else
                return context.getBuilder().CreateGEP(value, trueIdx);
        }
        else
        {
            if (ptr_type->isArrayTy())
                idx.push_back(idx_value);
            else
                return context.getBuilder().CreateGEP(value, idx_value);
        }
        return context.getBuilder().CreateInBoundsGEP(value, idx);
    }

    //stmt部分codegen

     llvm::Value *IfStmtNode::codegen(CodegenContext &context)
    {
        auto *cond = expr->codegen(context);
        if (!cond->getType()->isIntegerTy(1))       
            throw CodegenExcep("Incompatible type in if condition: expected boolean");

        //创建blcok
        auto *func = context.getBuilder().GetInsertBlock()->getParent();
        auto *then_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "then", func);
        auto *else_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "else");
        auto *cont_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
        context.getBuilder().CreateCondBr(cond, then_block, else_block);

        //插入语句
        context.getBuilder().SetInsertPoint(then_block);
        if_stmt->codegen(context);
        context.getBuilder().CreateBr(cont_block);

        func->getBasicBlockList().push_back(else_block);
        context.getBuilder().SetInsertPoint(else_block);
        if (else_stmt != nullptr)
            else_stmt->codegen(context);
        context.getBuilder().CreateBr(cont_block);

        func->getBasicBlockList().push_back(cont_block);
        context.getBuilder().SetInsertPoint(cont_block);
        return nullptr;
    }

    llvm::Value *WhileStmtNode::codegen(CodegenContext &context)
    {
        auto *func = context.getBuilder().GetInsertBlock()->getParent();
        auto *while_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "while", func);
        auto *loop_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "loop", func);
        auto *cont_block = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
        context.getBuilder().CreateBr(while_block);

        context.getBuilder().SetInsertPoint(while_block);
        auto *cond = expr->codegen(context);
        if (!cond->getType()->isIntegerTy(1))
        { throw CodegenExcep("Incompatible type in while condition: expected boolean"); }
        context.getBuilder().CreateCondBr(cond, loop_block, cont_block);

        context.getBuilder().SetInsertPoint(loop_block);
        stmt->codegen(context);
        context.getBuilder().CreateBr(while_block);

        func->getBasicBlockList().push_back(cont_block);
        context.getBuilder().SetInsertPoint(cont_block);
        return nullptr;
    }

    llvm::Value *ForStmtNode::codegen(CodegenContext &context)
    {
        if (!id->codegen(context)->getType()->isIntegerTy(32))
            throw CodegenExcep("Incompatible type in for iterator: expected int");

        auto init = make_new_node<AssignStmtNode>(id, init_val);
        auto upto = direction == ForDirection::To;
        auto cond = make_new_node<BinaryExprNode>(upto ? BinaryOp::Leq : BinaryOp::Geq, id, end_val);
        auto iter_stmt = make_new_node<AssignStmtNode>(id,
                make_new_node<BinaryExprNode>(upto ? BinaryOp::Plus :BinaryOp::Minus, id, make_new_node<IntegerNode>(1)));
        auto compound = make_new_node<CompoundStmtNode>();
        compound->merge(stmt); 
        compound->append(iter_stmt);
        auto while_stmt = make_new_node<WhileStmtNode>(cond, compound);

        init->codegen(context);
        while_stmt->codegen(context);
        return nullptr;
    }

    llvm::Value *RepeatStmtNode::codegen(CodegenContext &context)
    {
        auto init_value = make_new_node<IntegerNode>(1);
        auto init = make_new_node<AssignStmtNode>(id, init_value);
        auto cond = make_new_node<BinaryExprNode>(BinaryOp::Leq, id, expr);
        auto iter_stmt = make_new_node<AssignStmtNode>(id,
                make_new_node<BinaryExprNode>(BinaryOp::Plus, id, make_new_node<IntegerNode>(1)));
        auto compound = make_new_node<CompoundStmtNode>();
        compound->merge(stmt); 
        compound->append(iter_stmt);
        auto while_stmt = make_new_node<WhileStmtNode>(cond, compound);

        init->codegen(context);
        while_stmt->codegen(context);
        return nullptr;
    }

    llvm::Value *ProcStmtNode::codegen(CodegenContext &context)
    {
        return call->codegen(context);
    }

    llvm::Value *AssignStmtNode::codegen(CodegenContext &context)
    {
        llvm::Value *lhs;
        lhs = this->lhs->getAssignPtr(context);
        auto *rhs = this->rhs->codegen(context);
        auto *lhs_type = lhs->getType()->getPointerElementType();
        auto *rhs_type = rhs->getType();
        if (lhs_type->isDoubleTy() && rhs_type->isIntegerTy(32))
        {
            rhs = context.getBuilder().CreateSIToFP(rhs, context.getBuilder().getDoubleTy());
        }
        else if (lhs_type->isArrayTy())
        {
            if (!lhs_type->getArrayElementType()->isIntegerTy(8))
                throw CodegenExcep("Cannot assign to a non-char array");
            llvm::Value *zero = llvm::ConstantInt::getSigned(context.getBuilder().getInt32Ty(), 0);
            llvm::Value *lhsPtr = context.getBuilder().CreateInBoundsGEP(lhs, {zero, zero});;
            llvm::Value *rhsPtr;
            if (rhs_type->isPointerTy())
            {
                if (rhs_type->getPointerElementType()->isArrayTy())
                    rhsPtr = context.getBuilder().CreateInBoundsGEP(rhs, {zero, zero});
                else
                    rhsPtr = rhs;
                context.getBuilder().CreateCall(context.strcpyFunc, {lhsPtr, rhsPtr});
                return nullptr;
            }
            else if (rhs_type->isArrayTy())
            {
                if (is_ptr_of<IdNode>(this->rhs))
                    rhsPtr = context.getBuilder().CreateInBoundsGEP(cast_node<IdNode>(this->rhs)->getPtr(context), {zero, zero});
                else if (is_ptr_of<RecordRefNode>(this->rhs))
                    rhsPtr = cast_node<RecordRefNode>(this->rhs)->getPtr(context);
                else if (is_ptr_of<CustomProcNode>(this->rhs))
                    rhsPtr = context.getBuilder().CreateInBoundsGEP(rhs, {zero, zero});
                if (!rhs_type->getArrayElementType()->isIntegerTy(8))
                    throw CodegenExcep("Cannot assign to a non-char array");
                context.getBuilder().CreateCall(context.sprintfFunc, {lhsPtr, context.getBuilder().CreateGlobalStringPtr("%s"), rhsPtr});
                return nullptr;
            }
        }
        else if (lhs_type->isDoubleTy() && rhs_type->isIntegerTy())
        {
            auto *rhsFP = context.getBuilder().CreateSIToFP(rhs, lhs_type);
            context.getBuilder().CreateStore(rhsFP, lhs);
            return nullptr;
        }
        else if (lhs_type->isIntegerTy(32) && rhs_type->isDoubleTy())
        {
            auto *rhsSI = context.getBuilder().CreateFPToSI(rhs, lhs_type);
            std::cerr << "Warning: Assigning REAL type to INTEGER type, this may lose information" << std::endl;
            context.getBuilder().CreateStore(rhsSI, lhs);
            return nullptr;
        }
        if (!((lhs_type->isIntegerTy(1) && rhs_type->isIntegerTy(1))  // bool
                   || (lhs_type->isIntegerTy(32) && rhs_type->isIntegerTy(32))  // int
                   || (lhs_type->isIntegerTy(8) && rhs_type->isIntegerTy(8))  // char
                   || (lhs_type->isDoubleTy() && rhs_type->isDoubleTy()) // float
                   || (lhs_type->isArrayTy() && rhs_type->isIntegerTy(32)))) // string
            throw CodegenExcep("Incompatible type in assignment");
        context.getBuilder().CreateStore(rhs, lhs);
        return nullptr;
    }

    llvm::Value *SwitchStmtNode::codegen(CodegenContext &context)
    {
        auto *value = expr->codegen(context);
        if (!value->getType()->isIntegerTy())
            throw CodegenExcep("Incompatible type in case statement: expected char, integer");
        auto *func = context.getBuilder().GetInsertBlock()->getParent();
        auto *cont = llvm::BasicBlock::Create(context.getModule()->getContext(), "cont");
        auto *switch_inst = context.getBuilder().CreateSwitch(value, cont, static_cast<unsigned int>(branches.size()));
        for (auto &branch : branches)
        {
            llvm::ConstantInt *constant;
            if (is_ptr_of<ConstValueNode>(branch->branch))
                constant = llvm::cast<llvm::ConstantInt>(branch->branch->codegen(context));
            else // ID node
                constant = context.getConstInt(cast_node<IdNode>(branch->branch)->name);
            auto *block = llvm::BasicBlock::Create(context.getModule()->getContext(), "case", func);
            context.getBuilder().SetInsertPoint(block);
            branch->stmt->codegen(context);
            context.getBuilder().CreateBr(cont);
            switch_inst->addCase(constant, block);
        }
        func->getBasicBlockList().push_back(cont);
        context.getBuilder().SetInsertPoint(cont);
        return nullptr;
    }

 llvm::Type *SimpleTypeNode::findType(CodegenContext &context)
    {
        switch (type)
        {
            case Type::Bool: return context.getBuilder().getInt1Ty();
            case Type::Char: return context.getBuilder().getInt8Ty();
            case Type::Int: return context.getBuilder().getInt32Ty();
            case Type::Long: return context.getBuilder().getInt32Ty();
            case Type::Real: return context.getBuilder().getDoubleTy();
            default: throw CodegenExcep("Unknown");
        }
    }

    llvm::Type *AliasTypeNode::findType(CodegenContext &context) 
    {
        if (context.is_subroutine)
        {
            for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
            {
                llvm::Type *ret = nullptr;
                if ((ret = context.getAlias(*rit + "." + name->name)) != nullptr)
                    return ret;
            }
        }
        llvm::Type *ret = context.getAlias(name->name);
        if (ret == nullptr)
            throw CodegenExcep("Undefined alias in function " + context.getTrace() + ": " + name->name);
        return ret;
    }

    llvm::Type *StringTypeNode::findType(CodegenContext &context)
    {
        return llvm::ArrayType::get(context.getBuilder().getInt8Ty(), 256);
        // return nullptr;
    }

    llvm::Type *ArrayTypeNode::findType(CodegenContext &context)
    {
        auto st = this->range_start->codegen(context),
             ed = this->range_end->codegen(context);
        int s = llvm::cast<llvm::ConstantInt>(st)->getSExtValue(), 
            e = llvm::cast<llvm::ConstantInt>(ed)->getSExtValue();
        if (e < s)
            throw CodegenExcep("End index must be greater than start index!");
        return llvm::ArrayType::get(this->itemType->findType(context), e - s + 1);
    }

    void ArrayTypeNode::insertNestedArray(const std::string &outer, CodegenContext &context)
    {
        std::shared_ptr<ArrayTypeNode> a;
        std::shared_ptr<RecordTypeNode> rec;
        std::string aliasName;
        switch (itemType->type)
        {
        case Type::Alias:
            aliasName = cast_node<AliasTypeNode>(itemType)->name->name;
            for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                if ((a = context.getArrayAlias(*rit + "." + aliasName)) != nullptr)
                    break;
            if (a == nullptr) a = context.getArrayAlias(aliasName);
            if (a != nullptr)
            {
                if (!context.setArrayEntry(outer + "[]", a)) throw CodegenExcep("Duplicate nested array field!");
                a->insertNestedArray(outer + "[]", context);
                break;
            }
            for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
                if ((rec = context.getRecordAlias(*rit + "." + aliasName)) != nullptr)
                    break;
            if (rec == nullptr) rec = context.getRecordAlias(aliasName);
            if (rec == nullptr) break;
            break;
        case Type::String:
            if (!context.setArrayEntry(outer + "[]", 0, 255)) throw CodegenExcep("Duplicate nested array field!");
            break;
        case Type::Array:
            a = cast_node<ArrayTypeNode>(itemType);
            assert(a != nullptr);
            if (!context.setArrayEntry(outer + "[]", a)) throw CodegenExcep("Duplicate nested array field!");
            a->insertNestedArray(outer + "[]", context);
            break;
        default:
            break;
        }
    }

    void RecordTypeNode::append(const std::shared_ptr<VarDeclNode> &var)
    {
       
    }
    void RecordTypeNode::merge(const std::shared_ptr<RecordTypeNode> &rhs)
    {
        
    }
    void RecordTypeNode::merge(std::shared_ptr<RecordTypeNode> &&rhs)
    {
        
    }

    llvm::Type *RecordTypeNode::findType(CodegenContext &context)
    { 
        std::vector<llvm::Type *> fieldTy;
        
        return llvm::StructType::get(context.getBuilder().getContext(), fieldTy);   
    }
    void RecordTypeNode::insertNestedRecord(const std::string &outer, CodegenContext &context)
    {
       
    }

    llvm::Value *RecordTypeNode::getFieldIdx(const std::string &name, CodegenContext &context)
    {
       
        return nullptr;
    }

    llvm::Type *ConstValueNode::findType(CodegenContext &context)
    {
        switch (type) 
        {
            case Type::Bool: return context.getBuilder().getInt1Ty();
            case Type::Int: return context.getBuilder().getInt32Ty();
            case Type::Long: return context.getBuilder().getInt32Ty();
            case Type::Char: return context.getBuilder().getInt8Ty();
            case Type::Real: return context.getBuilder().getDoubleTy();
            case Type::String: throw CodegenExcep("String currently not supported.");
            default: throw CodegenExcep("Unknown "); return nullptr;
        }
        return nullptr;
    }

    llvm::Value *BooleanNode::codegen(CodegenContext &context)
    {
        return val ? context.getBuilder().getTrue() : context.getBuilder().getFalse();
    }

    llvm::Value *IntegerNode::codegen(CodegenContext &context)
    {
        auto *ty = context.getBuilder().getInt32Ty();
        return llvm::ConstantInt::getSigned(ty, val);
    }

    llvm::Value *CharNode::codegen(CodegenContext &context)
    {
        auto *ty = context.getBuilder().getInt8Ty();
        return llvm::ConstantInt::getSigned(ty, val);
    }

    llvm::Value *RealNode::codegen(CodegenContext &context)
    {
        auto *ty = context.getBuilder().getDoubleTy();
        return llvm::ConstantFP::get(ty, val);
    }

    llvm::Value *StringNode::codegen(CodegenContext &context) 
    {
        llvm::Module *M = context.getModule().get();
        llvm::LLVMContext& ctx = M->getContext();
        llvm::Constant *strConstant = llvm::ConstantDataArray::getString(ctx, val);
        llvm::Type *t = strConstant->getType();
        llvm::GlobalVariable *GVStr = new llvm::GlobalVariable(*M, t, true, llvm::GlobalValue::ExternalLinkage, strConstant, "");
        llvm::Constant* zero = llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(ctx));

        llvm::Constant *strVal = llvm::ConstantExpr::getGetElementPtr(t, GVStr, zero, true);

        return strVal;
    }

    llvm::Type *VoidTypeNode::findType(CodegenContext &context)
    {
        return context.getBuilder().getVoidTy();
    }

    llvm::Value *IdNode::codegen(CodegenContext &context)
    {
        llvm::Constant *cv;
        if ((cv = this->getConstVal(context)) != nullptr)
            return cv;
        return context.getBuilder().CreateLoad(getPtr(context));//CreateLoad取变量值
    }
    llvm::Constant *IdNode::getConstVal(CodegenContext &context)
    {
        llvm::Constant *value = nullptr;
        for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
        {
            if ((value = context.getConstVal(*rit + "." + name)) != nullptr)
                break;
        }
        if (value == nullptr) value = context.getConstVal(name);
        return value;
    }
    llvm::Value *IdNode::getPtr(CodegenContext &context)
    {
        llvm::Value *value = nullptr;
        for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
        {
            if ((value = context.getLocal(*rit + "." + name)) != nullptr)
                break;
            else if (context.getConst(*rit + "." + name) != nullptr)
            {
                value = context.getModule()->getGlobalVariable(*rit + "." + name);
                break;
            }
        }
        if (value == nullptr) value = context.getModule()->getGlobalVariable(name);
        if (value == nullptr) throw CodegenExcep("Identifier not found in function " + context.getTrace() + ": " + name);
        return value;
    }
    llvm::Value *IdNode::getAssignPtr(CodegenContext &context)
    {
        if (context.getConst(name) != nullptr)
            throw CodegenExcep("Cannot assign to a const value!");
        llvm::Value *value = nullptr;
        for (auto rit = context.traces.rbegin(); rit != context.traces.rend(); rit++)
        {
            if ((value = context.getLocal(*rit + "." + name)) != nullptr)
            {
                if (context.getConst(*rit + "." + name) != nullptr)
                    throw CodegenExcep("Cannot assign to a const value!");
                break;
            }
        }
        if (value == nullptr) value = context.getModule()->getGlobalVariable(name);
        if (value == nullptr) throw CodegenExcep("Identifier not found in function " + context.getTrace() + ": " + name);
        return value;
    }

} 

