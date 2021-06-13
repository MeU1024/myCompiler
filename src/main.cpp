#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include "Vis.hpp"
#include "basis.hpp"
#include "context.hpp"
#include "parser.hpp"

extern FILE *yyin;
std::shared_ptr<fpc::ProgramNode> program;

int main(int argc, char* argv[])
{
    bool opt = false;

    if ((yyin = fopen(argv[1], "r")) == nullptr)
    {
        std::cerr << "Error: cannot open iutput file" << std::endl;
        exit(1);
    }

    //词法语法分析
    fpc::parser myparser;

    try
    {
        myparser.parse();
    }
    catch(const std::invalid_argument& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "error during scanning" << std::endl;
        exit(1);
    }
    catch(const std::logic_error& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "error during parsing" << std::endl;
        exit(1);
    }

    std::cout << "Scan & Parse successfully! " << std::endl;

    //AST可视化
    std::string astVisName = argv[1];
    astVisName.erase(astVisName.rfind('.'));
    astVisName.append(".dot");
    fpc::ASTvis astVis(astVisName);
    astVis.travAST(program);
    std::cout << " Generate AST successfully！See result in AST.dot！ " << std::endl;


    //中间代码生成
    fpc::CodegenContext genContext("main", opt);
    try 
    {
        program->codegen(genContext);
    } 
    catch (fpc::CodegenExcep &e) 
    {
        std::cerr << "[CODEGEN ERROR] ";
        std::cerr << e.what() << std::endl;
        std::cerr << "error during code generation" << std::endl;
        abort();
    }


    std::cout << "Code generation completed!" << std::endl;

    std::string output;
    output = argv[1];
    output.erase(output.rfind('.'));
    output.append(".ll"); 

    std::error_code ec;
    llvm::raw_fd_ostream fd(output, ec, llvm::sys::fs::F_None);
    if (ec)
    { 
        llvm::errs() << "Could not open file: " << ec.message(); 
        exit(1); 
    }

    genContext.getModule()->print(fd, nullptr); 

    std::cout << "Compile result output: " << output << std::endl;

    return 0;
}
