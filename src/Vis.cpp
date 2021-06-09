#include "Vis.hpp"
#include "basis.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <list>
using namespace fpc;

void fpc::ASTvis::AddNode(std::string label){
    int id = id_cnt++;
    of << "node_" << id << "[";
    of << "label=\"" << label << "\"];\n";

    if (id != 0) {
        of << "node_" << stk_.top() << "->"
              << "node_" << id << ";\n";
    }
    stk_.push(id);
}

void fpc::ASTvis::travAST(const std::shared_ptr<ProgramNode>& prog)
{
     of << "digraph G{\n";
    travProgram(prog);
    stk_.pop();
    of <<"}";

    return;
    
}

int fpc::ASTvis::travProgram(const std::shared_ptr<fpc::ProgramNode>& prog)
{
    AddNode("program");
    travRoutineBody(fpc::cast_node<fpc::BaseRoutineNode>(prog));
    return 0;
}

int fpc::ASTvis::travRoutineBody(const std::shared_ptr<fpc::BaseRoutineNode>& prog)
{
    int tmp = 0;
    AddNode("CONST");
    tmp = travCONST(prog->header->constList);
    stk_.pop();

    AddNode("TYPE");
    tmp = travTYPE(prog->header->typeList);
    stk_.pop();

    AddNode("VAR");
    tmp = travVAR(prog->header->varList);
    stk_.pop();

    AddNode("PROC_FUNC");
    tmp = travSubprocList(prog->header->subroutineList);
    stk_.pop();

    AddNode("STMT");
    tmp = travCompound(prog->body);
    stk_.pop();

    return 0;
}

//TODO:
int fpc::ASTvis::travCONST(const std::shared_ptr<fpc::ConstDeclList>& const_declListAST)
{
    
    return 0;
    
}

int fpc::ASTvis::travTYPE(const std::shared_ptr<fpc::TypeDeclList>& type_declListAST)
{
    
}

int fpc::ASTvis::travVAR(const std::shared_ptr<fpc::VarDeclList>& var_declListAST)
{
    std::list<std::shared_ptr<VarDeclNode>>& varList(var_declListAST->getChildren());
    int lines = varList.size();

    std::string label;
    for (auto &p : varList) {
            label = "VAR_";
            switch (p->type->type) {
            case fpc::Type::Void    : label.append("VOID:") ; break;
            case fpc::Type::Array   : label.append("ARRAY:")  ; break;
            case fpc::Type::Record  : label.append("RECORD:") ; break;
            case fpc::Type::Bool : label.append("BOOLEAN:"); break;
            case fpc::Type::Int : label.append("INTEGER:"); break;
            case fpc::Type::Long : label.append("LONG:"); break;
            case fpc::Type::Real    : label.append("REAL:")   ; break;
            case fpc::Type::String  : label.append("STRING:") ; break;
            default : label.append("ERROR:"); break;
            }
            label.append(p->name->name);
            AddNode(label);
            stk_.pop();
    }

    return 0;
}

int fpc::ASTvis::travSubprocList(const std::shared_ptr<fpc::RoutineList>& subProc_declListAST)
{
    std::list<std::shared_ptr<fpc::RoutineNode>>& progList(subProc_declListAST->getChildren());
    int tmp = 0, lines = progList.size();

    for (auto &p : progList) {
        
        tmp = travSubproc(p);

    }
    return 0;
   
}

int fpc::ASTvis::travSubproc(const std::shared_ptr<fpc::RoutineNode>& subProc_AST)
{
    
}

int fpc::ASTvis::travCompound(const std::shared_ptr<fpc::CompoundStmtNode>& compound_declListAST)
{
     if (compound_declListAST == nullptr) return 0;
    std::list<std::shared_ptr<fpc::StmtNode>>& stmtList(compound_declListAST->getChildren());
    int tmp = 0, lines = stmtList.size();
    for (auto &p : stmtList) {
        
        if (fpc::is_ptr_of<fpc::IfStmtNode>(p))
        {
          AddNode("If Statement");
            tmp += travStmt(fpc::cast_node<fpc::IfStmtNode>(p));
            stk_.pop();
        
        }
        else if (fpc::is_ptr_of<fpc::WhileStmtNode>(p))
        {
        }
        else if (fpc::is_ptr_of<fpc::ForStmtNode>(p))
        {
            AddNode("For Statement");
            tmp += travStmt(fpc::cast_node<fpc::ForStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::RepeatStmtNode>(p))
        {
            
        }
        else if (fpc::is_ptr_of<fpc::ProcStmtNode>(p))
        {
           AddNode("Proc Statement");
            tmp += travStmt(fpc::cast_node<fpc::ProcStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::AssignStmtNode>(p))
        {
            AddNode("Assign Statement");
            tmp += travStmt(fpc::cast_node<fpc::AssignStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::CaseStmtNode>(p))
        {
           
    
        }
 
    }
    return 0;
}
/*
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::CaseStmtNode>&p_stmp)
{
   
}
*/
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::StmtNode>&p_stmp)
{
    
}
// * done
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::IfStmtNode>&p_stmp)
{
     if (p_stmp == nullptr) return 0;
    int tmp = 0, lines = 3;
   // AddNode("If Statement ",0,0);
    tmp += travExpr(p_stmp->expr);
    

    //for (int i=0; i<tmp+1; ++i) of << texNone;
    lines += tmp; tmp = 0;
    // std::cout << "debug info: IF expr over" << std::endl;
    //of << "child { node {IF Statment if stmt}\n";
    tmp = travCompound(p_stmp->if_stmt);
    //of << "}\n";
    //for (int i=0; i<tmp; ++i) of << texNone;
    lines += tmp; tmp = 0;

    // std::cout << "debug info: IF part over" << std::endl;
   // of << "child { node {IF Statment else stmt}\n";
    tmp = travCompound(p_stmp->else_stmt);
    //of << "}\n";
    //for (int i=0; i<tmp; ++i) of << texNone;
    lines += tmp; tmp = 0;
    // std::cout << "debug info: ELSE part over" << std::endl;
   // stk_.pop();


    return 0;
}
/*
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::WhileStmtNode>&p_stmp)
{
    
}
*/
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::ForStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return 0;
    int tmp = 0, lines = 0;
    //of << "child { node {FOR Expr: ";
    //of << p_stmp->id->name << " }\n ";

    tmp += travExpr(p_stmp->init_val);
    // of << texNone;
    tmp += travExpr(p_stmp->end_val);
    //of << "}\n";
    for (int i=0; i<tmp+1; ++i) of << texNone;
    lines += tmp; tmp = 0;

    //of << "child { node {FOR Statment}\n";
    tmp = travCompound(p_stmp->stmt);
    //of << "}\n";
    for (int i=0; i<tmp; ++i) of << texNone;
    lines += tmp;

    return lines;
}
/*
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::RepeatStmtNode>&p_stmp)
{
    
}
*/
int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::ProcStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return 0;
    int tmp = 0, lines = 0;
    //of << "child { node {CALL Statment";
    // tmp = travExpr(p_stmp->expr);
    // for (int i=0; i<tmp; ++i) of << texNone;
    //AddNode("Call Statement",0,0);
    //stk_.pop();
    lines += tmp;
    //of << "}\n}\n";
    return lines;
}

int fpc::ASTvis::travStmt(const std::shared_ptr<fpc::AssignStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return 0;
    int tmp = 2, lines = 0;
    
   
    tmp += travExpr(p_stmp->lhs);
    tmp += travExpr(p_stmp->rhs);
   // of << "}\n";
    for (int i=0; i<tmp; ++i) 
    //of << texNone;
    lines += tmp;
    // of << "}\n}\n";
    return lines;
}

int fpc::ASTvis::travExpr(const std::shared_ptr<ExprNode>& expr)
{
    int tmp = 0, lines = 0;
    if (fpc::is_ptr_of<fpc::BinaryExprNode>(expr))
        tmp += travExpr(fpc::cast_node<fpc::BinaryExprNode>(expr));
    // tmp += travExpr(fpc::cast_node<fpc::UnaryExprNode>(expr));
    else if (fpc::is_ptr_of<fpc::IdentifierNode>(expr))
        tmp += travExpr(fpc::cast_node<IdentifierNode>(expr));
    else if (fpc::is_ptr_of<fpc::ConstValueNode>(expr))
        tmp += travExpr(fpc::cast_node<ConstValueNode>(expr));
    else if (fpc::is_ptr_of<fpc::ArrayRefNode>(expr))
        tmp += travExpr(fpc::cast_node<fpc::ArrayRefNode>(expr));
   // else if (fpc::is_ptr_of<fpc::RecordRefNode>(expr))
    //    tmp += travExpr(fpc::cast_node<fpc::RecordRefNode>(expr));
    //else if (fpc::is_ptr_of<fpc::CustomProcNode>(expr))
     //   tmp += travExpr(fpc::cast_node<fpc::CustomProcNode>(expr));
    else if (fpc::is_ptr_of<fpc::SysProcNode>(expr))
        tmp += travExpr(fpc::cast_node<fpc::SysProcNode>(expr));
   
    return lines;
}

int fpc::ASTvis::travExpr(const std::shared_ptr<BinaryExprNode>& expr)
{
    if (expr == nullptr) return 0;
    int tmp = 0, lines = 2;

    std::string label;
    label.append("Binary:");
    switch (expr->op)
    {
        case fpc::BinaryOp::Eq: label.append("==");break;
        case fpc::BinaryOp::Neq: label.append("!=");break;
        case fpc::BinaryOp::Leq: label.append("<=");break;
        case fpc::BinaryOp::Geq: label.append(">=");break;
        case fpc::BinaryOp::Lt: label.append("<");break;
        case fpc::BinaryOp::Gt: label.append(">");break;
        case fpc::BinaryOp::Plus: label.append("+");break;
        case fpc::BinaryOp::Minus: label.append("-");break;
        case fpc::BinaryOp::Truediv: label.append("/");break;
        case fpc::BinaryOp::Div: label.append("//");break;
        case fpc::BinaryOp::Mod: label.append("\%");break;
        case fpc::BinaryOp::Mul: label.append("*");break;
        case fpc::BinaryOp::Or:  label.append("|");break;
        case fpc::BinaryOp::And: label.append("&");break;
        case fpc::BinaryOp::Xor: label.append("^");break;
        default: label.append("ERROR"); break;
    }
    AddNode(label);
    tmp += travExpr(expr->lhs);
    tmp += travExpr(expr->rhs);
    stk_.pop();
    lines += tmp;

    return 0;
    
}

int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::UnaryExprNode>& expr)
{
    
}

int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::ArrayRefNode>& expr)
{
   if (expr == nullptr) return 0;
    int tmp = 0, lines = 0;
   // of << "child { node {ARRAY REFERENCE";
    // tmp = travExpr(p_stmp->expr);
    //for (int i=0; i<tmp; ++i) of << texNone;
    AddNode("Array",0,0);
    stk_.pop();
        lines += tmp;
   // of << "}\n}\n";
    return lines;
}
/*
int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::RecordRefNode>& expr)
{
    
}
*/
int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::ProcNode>& expr)
{
    if (expr == nullptr) return 0;
    int tmp = 0, lines = 0;
    return lines;
}
/*
int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::CustomProcNode>& expr)
{
    
}
*/
int fpc::ASTvis::travExpr(const std::shared_ptr<fpc::SysProcNode>& expr)
{
    if (expr == nullptr) return 0;
    int tmp = 0, lines = 0;

    //of << "child { node {SysFunc: ";

    std::string label;
    label.append("SysFunc:");
    switch (expr->name)
    {
        case fpc::SysFunc::Read:  label.append("read()"); break;
        case fpc::SysFunc::Write: label.append("write()"); break;
        case fpc::SysFunc::Writeln:label.append( "writeln()");  break;
        case fpc::SysFunc::Abs: label.append( "abs()"); break;
        case fpc::SysFunc::Chr: label.append( "chr()"); break;
        case fpc::SysFunc::Odd: label.append( "odd()"); break;
        case fpc::SysFunc::Ord: label.append( "ord()"); break;
        case fpc::SysFunc::Pred: label.append( "pred()"); break;
        case fpc::SysFunc::Sqr: label.append( "sqr()"); break;
        case fpc::SysFunc::Sqrt: label.append( "sqrt()"); break;
        case fpc::SysFunc::Succ: label.append( "succ()"); break;
    }
    AddNode(label);
    stk_.pop();
    return lines;
}
