#ifndef __VIS__H__
#define __VIS__H__

#include "basis.hpp"

#include <fstream>
#include <stack>

namespace fpc {
    class ASTvis {
    public:
        ASTvis(const std::string &output = "AST.dot") : of(output, of.trunc | of.out)
        {
            if (!of.is_open())
            {
                std::cerr << "failed to open file " << output << std::endl;
                exit(1);
            }
        }
        ~ASTvis() = default;
        void travAST(const std::shared_ptr<ProgramNode>& prog);
        void AddNode(std::string label);
    private:
        void travProgram(const std::shared_ptr<ProgramNode>& prog);
        void travRoutineBody(const std::shared_ptr<BaseRoutineNode>& prog);

        void travInit(const std::shared_ptr<InitNode>& InitAST);
        void travCONST(const std::shared_ptr<ConstDeclList>& const_declListAST);
        void travTYPE(const std::shared_ptr<TypeDeclList>& type_declListAST);
        void travVAR(const std::shared_ptr<VarDeclList>& var_declListAST);
        void travSubprocList(const std::shared_ptr<RoutineList>& subProc_declListAST);
        void travSubproc(const std::shared_ptr<RoutineNode>& subProc_AST);
        void travCompound(const std::shared_ptr<CompoundStmtNode>& compound_declListAST);

        void travStmt(const std::shared_ptr<StmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<IfStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<WhileStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<ForStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<RepeatStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<ProcStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<AssignStmtNode>&p_stmp);
        void travStmt(const std::shared_ptr<SwitchStmtNode>&p_stmp);

        void travExpr(const std::shared_ptr<ExprNode>& expr);
        void travExpr(const std::shared_ptr<BinaryExprNode>& expr);
        void travExpr(const std::shared_ptr<fpc::IdNode>& expr);
        void travExpr(const std::shared_ptr<fpc::ConstValueNode>& expr);
        void travExpr(const std::shared_ptr<ArrayRefNode>& expr);
        void travExpr(const std::shared_ptr<RecordRefNode>& expr);
        void travExpr(const std::shared_ptr<ProcNode>& expr);
        void travExpr(const std::shared_ptr<CustomProcNode>& expr);
        void travExpr(const std::shared_ptr<SysProcNode>& expr);

        int node_cnt    = 0;
        int subproc_cnt = 0;
        int stmt_cnt    = 0;
        int leaf_cnt    = 0;
        int id_cnt = 0;
        std::stack<int> stk_;
        std::fstream of;
 
    };
}


#endif

