#include "Vis.hpp"

#include <fstream>
#include <iostream>
#include <string>


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

void fpc::ASTvis::travProgram(const std::shared_ptr<fpc::ProgramNode>& prog)
{
    AddNode("program");
    travRoutineBody(fpc::cast_node<fpc::BaseRoutineNode>(prog));
}


void fpc::ASTvis::travRoutineBody(const std::shared_ptr<fpc::BaseRoutineNode>& prog)
{
    AddNode("INIT");
    //travInit(prog->init_part);
    stk_.pop();

    AddNode("CONST");
    travCONST(prog->routine_head->constList);
    stk_.pop();

    AddNode("TYPE");
    travTYPE(prog->routine_head->typeList);
    stk_.pop();

    AddNode("VAR");
    travVAR(prog->routine_head->varList);
    stk_.pop();

    AddNode("PROC_FUNC");
    travSubprocList(prog->routine_head->subroutineList);
    stk_.pop();

    AddNode("STMT");
    travCompound(prog->routine_body);
    stk_.pop();

    return;
}

void fpc::ASTvis::travInit(const std::shared_ptr<InitNode>& InitAST){
    
    if (InitAST->content != ""){
    AddNode(InitAST->content);
    stk_.pop();
    }
    
    return;
}

void fpc::ASTvis::travCONST(const std::shared_ptr<fpc::ConstDeclList>& const_declListAST)
{
    std::list<std::shared_ptr<ConstDeclNode>>& constList(const_declListAST->getChildren());
    std::string label;
    for (auto &p : constList) {
            label = "CONST_";
            switch (p->val->type) {
            case fpc::Type::Void    : of << "VOID:"   ; break;
            case fpc::Type::Array   : of << "ARRAY:"  ; break;
            case fpc::Type::Record  : of << "RECORD:" ; break;
            case fpc::Type::Bool : of << "BOOLEAN:"; break;
            case fpc::Type::Int : of << "INTEGER:"; break;
            case fpc::Type::Long : of << "LONG:"; break;
            case fpc::Type::Real    : of << "REAL:"   ; break;
            case fpc::Type::String  : of << "STRING:" ; break;
            default : label.append("ERROR:"); break;
            }
            label.append(p->name->name);
            AddNode(label);
            stk_.pop();
    } 
    return ;
}

void fpc::ASTvis::travTYPE(const std::shared_ptr<fpc::TypeDeclList>& type_declListAST)
{
    std::list<std::shared_ptr<TypeDeclNode>>& typeList(type_declListAST->getChildren());
    std::string label;
    for (auto &p : typeList) {
            label = "TYPE_";
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
    return;
}

void fpc::ASTvis::travVAR(const std::shared_ptr<fpc::VarDeclList>& var_declListAST)
{
    std::list<std::shared_ptr<VarDeclNode>>& varList(var_declListAST->getChildren());
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
    return;
}

void fpc::ASTvis::travSubprocList(const std::shared_ptr<fpc::RoutineList>& subProc_declListAST)
{
    std::list<std::shared_ptr<fpc::RoutineNode>>& progList(subProc_declListAST->getChildren());

    for (auto &p : progList) {     
        travSubproc(p);
    }
    return;
}

void fpc::ASTvis::travSubproc(const std::shared_ptr<fpc::RoutineNode>& subProc_AST)
{
    std::string label;
    if (subProc_AST->retType->type == fpc::Type::Void ){
        label.append("PROCEDURE:");
        label.append(subProc_AST->getName());
        AddNode(label);
    }
    else
    {
        label.append("FUNCTION:");
        label.append(subProc_AST->getName());
        label.append("\nRET_TYPE:");
        //of << "$ ---- $RET$-$TYPE: ";
        switch (subProc_AST->retType->type) {
            case fpc::Type::Void    : label.append("VOID") ; break;
            case fpc::Type::Array   :  label.append("ARRAY") ; break;
            case fpc::Type::Record  :  label.append("RECORD") ; break;
            case fpc::Type::Bool :  label.append( "BOOLEAN"); break;
            case fpc::Type::Int :  label.append( "INTEGER"); break;
            case fpc::Type::Long :  label.append("LONG"); break;
            case fpc::Type::Real    :  label.append("REAL")   ; break;
            case fpc::Type::String  :  label.append( "STRING") ; break;
            default :  label.append("ERROR"); break;
        }
    }

    std::list<std::shared_ptr<ParamNode>>& paramAsts
            = subProc_AST->params->getChildren();
    {
         label.append("\nPARAMS:");
        for (auto &p : paramAsts) {
          label.append(p->name->name); 
          label.append(":");
            switch (p->type->type) {
                case fpc::Type::Void    : label.append("VOID")   ; break;
                case fpc::Type::Array   : label.append("ARRAY")   ; break;
                case fpc::Type::Record  : label.append("RECORD")  ; break;
                case fpc::Type::Bool : label.append("BOOLEAN") ; break;
                case fpc::Type::Int : label.append("INTEGER") ; break;
                case fpc::Type::Long : label.append("LONG") ; break;
                case fpc::Type::Real    : label.append("REAL" )   ; break;
                case fpc::Type::String  : label.append("STRING")  ; break;
                default : label.append("ERROR") ; break;
            }
        label.append("\n");
        }
    }
    AddNode(label);
    travRoutineBody(fpc::cast_node<fpc::BaseRoutineNode>(subProc_AST));
    stk_.pop();

    return;
}

void fpc::ASTvis::travCompound(const std::shared_ptr<fpc::CompoundStmtNode>& compound_declListAST)
{

    if (compound_declListAST == nullptr) return;
    std::list<std::shared_ptr<fpc::StmtNode>>& stmtList(compound_declListAST->getChildren());
    for (auto &p : stmtList) {
        
        if (fpc::is_ptr_of<fpc::IfStmtNode>(p))
        {
          AddNode("If Statement");
            travStmt(fpc::cast_node<fpc::IfStmtNode>(p));
            stk_.pop();
        
        }
        else if (fpc::is_ptr_of<fpc::WhileStmtNode>(p))
        {
            AddNode("While Statement");
            travStmt(fpc::cast_node<fpc::WhileStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::ForStmtNode>(p))
        {
            AddNode("For Statement");
            travStmt(fpc::cast_node<fpc::ForStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::RepeatStmtNode>(p))
        {
            AddNode("Repeat Statement");
            travStmt(fpc::cast_node<fpc::RepeatStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::ProcStmtNode>(p))
        {
           AddNode("Proc Statement");
            travStmt(fpc::cast_node<fpc::ProcStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::AssignStmtNode>(p))
        {
            AddNode("Assign Statement");
            travStmt(fpc::cast_node<fpc::AssignStmtNode>(p));
            stk_.pop();
        }
        else if (fpc::is_ptr_of<fpc::SwitchStmtNode>(p))
        {
            AddNode("Case Statement");
            travStmt(fpc::cast_node<fpc::SwitchStmtNode>(p));
            stk_.pop();
        }

    }
    return;
}

//TODO:
void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::SwitchStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
    std::list<std::shared_ptr<fpc::CaseBranchNode>>& stmtList(p_stmp->branches);
    travExpr(p_stmp->expr);
    int sublines = stmtList.size();
    for (auto &p : stmtList)
    {
        std::string br;
        if (fpc::is_ptr_of<fpc::IntegerNode>(p->branch))
            br = std::to_string(fpc::cast_node<IntegerNode>(p->branch)->val);
        else if (fpc::is_ptr_of<fpc::IdNode>(p->branch))
            br = fpc::cast_node<fpc::IdNode>(p->branch)->name;
        travCompound(p->stmt);

    }

    return;
}

void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::StmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
    return ;
}
// * done
void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::IfStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
  
    travExpr(p_stmp->expr);
    travCompound(p_stmp->if_stmt);
    travCompound(p_stmp->else_stmt);

    return;
}
void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::WhileStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
    travExpr(p_stmp->expr);
    travCompound(p_stmp->stmt);
    return;
}

void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::ForStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
    AddNode("init value");
    travExpr(p_stmp->init_val);
    stk_.pop();
    AddNode("end value");
    travExpr(p_stmp->end_val);
    stk_.pop();
    AddNode("For Body");
    travCompound(p_stmp->stmt);
    stk_.pop();

    return;
}
void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::RepeatStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
    AddNode("Repeat Times");
    travExpr(p_stmp->expr);
    stk_.pop();
    AddNode("Counter");
    travExpr(p_stmp->id);
    stk_.pop();
    AddNode("Repeat Body");
    travCompound(p_stmp->stmt);
    stk_.pop();

    return;
}
void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::ProcStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
   
    return;
}

void fpc::ASTvis::travStmt(const std::shared_ptr<fpc::AssignStmtNode>&p_stmp)
{
    if (p_stmp == nullptr) return;
   travExpr(p_stmp->lhs);
    travExpr(p_stmp->rhs);
    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<ExprNode>& expr)
{
    if (fpc::is_ptr_of<fpc::BinaryExprNode>(expr))
        travExpr(fpc::cast_node<fpc::BinaryExprNode>(expr));
    else if (fpc::is_ptr_of<fpc::IdNode>(expr))
        travExpr(fpc::cast_node<IdNode>(expr));
    else if (fpc::is_ptr_of<fpc::ConstValueNode>(expr))
        travExpr(fpc::cast_node<ConstValueNode>(expr));
    else if (fpc::is_ptr_of<fpc::ArrayRefNode>(expr))
        travExpr(fpc::cast_node<fpc::ArrayRefNode>(expr));
    else if (fpc::is_ptr_of<fpc::RecordRefNode>(expr))
        travExpr(fpc::cast_node<fpc::RecordRefNode>(expr));
    else if (fpc::is_ptr_of<fpc::CustomProcNode>(expr))
        travExpr(fpc::cast_node<fpc::CustomProcNode>(expr));
    else if (fpc::is_ptr_of<fpc::SysProcNode>(expr))
       travExpr(fpc::cast_node<fpc::SysProcNode>(expr));
    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<BinaryExprNode>& expr)
{
    if (expr == nullptr) return;
    std::string label;
    label.append("Binary:");
    switch (expr->op)
    {
        case fpc::BinaryOp::Eq: label.append("==");break;
        case fpc::BinaryOp::Neq: label.append("<>");break;
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
    travExpr(expr->lhs);
    travExpr(expr->rhs);
    stk_.pop();

    return;
}


void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::ConstValueNode>& expr)
{
    if (expr == nullptr) return;
    AddNode("VALUE");
    stk_.pop();

    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::IdNode>& expr)
{
    if (expr == nullptr) return;
   
    AddNode(expr->name);
    stk_.pop();

    return;
}

//TODO:TEST
void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::ArrayRefNode>& expr)
{
    if (expr == nullptr) return;
    AddNode("Array");
    stk_.pop();

    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::RecordRefNode>& expr)
{
    if (expr == nullptr) return;
    AddNode("Record");
    stk_.pop();
    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::ProcNode>& expr)
{
    if (expr == nullptr) return;
    return;
}

//TODO:
void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::CustomProcNode>& expr)
{
    if (expr == nullptr) return;
    return;
}

void fpc::ASTvis::travExpr(const std::shared_ptr<fpc::SysProcNode>& expr)
{
    if (expr == nullptr) return;
   
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
    return;

}

