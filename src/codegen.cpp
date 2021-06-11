#include "basis.hpp"
#include "codegen_context.hpp"
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
       

        return nullptr;
    }

    //TODO:RoutineNode？

    llvm::Value *RoutineNode::codegen(CodegenContext &context)
    {
        context.log() << "Entering function " + name->name << std::endl;

        if (context.getModule()->getFunction(name->name) != nullptr)
            throw CodegenException("Duplicate function definition: " + name->name);

        context.traces.push_back(name->name);

        std::vector<llvm::Type *> types;
        std::vector<std::string> names;


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
            auto *type = retType->getLLVMType(context);

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

       

        return nullptr;
    }

     llvm::Value *VarDeclNode::createGlobalArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
        return;
    }

    llvm::Value *VarDeclNode::createArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &arrTy)
    {
    

        return;
    }
    
    llvm::Value *VarDeclNode::codegen(CodegenContext &context)
    {
       
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
        }
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
                        func_args.push_back(context.getBuilder().CreateGlobalStringPtr("%10d"));
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
                        // else if (is_ptr_of<RecordRefNode>(arg))
                        // {
                        //     auto argId = cast_node<RecordRefNode>(arg);
                        //     valuePtr = argId->getPtr(context);
                        // }
                        // else if (is_ptr_of<ArrayRefNode>(arg))
                        // {
                        //     auto argId = cast_node<ArrayRefNode>(arg);
                        //     valuePtr = argId->getPtr(context);
                        // }
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
                    // else if (is_ptr_of<ArrayRefNode>(arg))
                    //     ptr = cast_node<ArrayRefNode>(arg)->getPtr(context);
                    // else if (is_ptr_of<RecordRefNode>(arg))
                    //     ptr = cast_node<RecordRefNode>(arg)->getPtr(context);
                    // else if (is_ptr_of<ArrayRefNode>(arg))
                    //     ptr = cast_node<ArrayRefNode>(arg)->getPtr(context);
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
        
    }

   


} // namespace fpc
