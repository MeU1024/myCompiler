#ifndef BASIS_AST
#define BASIS_AST
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

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <list>
#include <memory>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <set>

namespace fpc
{
    class CodegenContext;
    class BasicNode
    {
    public:
        BasicNode() {}
        ~BasicNode() {}
        virtual llvm::Value *codegen(CodegenContext &) = 0;
    };

    class InitNode
    {
    public:
        std::string content;
        InitNode(const std::string &str = "")
            : content(str) 
        {
        }
        ~InitNode() = default;

        llvm::Value *codegen(CodegenContext &context) ;
    };

    template<typename T, typename U>
    inline typename std::enable_if<std::is_base_of<BasicNode, U>::value && std::is_base_of<BasicNode, T>::value, bool>::type 
    is_ptr_of(const std::shared_ptr<U> &ptr)
    {
        return dynamic_cast<T *>(ptr.get()) != nullptr;
    }

    template<typename T, typename U>
    inline typename std::enable_if<std::is_base_of<BasicNode, U>::value && std::is_base_of<BasicNode, T>::value, std::shared_ptr<T>>::type
    cast_node(const std::shared_ptr<U> &ptr)
    {
        return std::dynamic_pointer_cast<T>(ptr);
    }

    //模板类list
    template <typename T>
    class ListNode: public BasicNode
    {
    private:
        std::list<std::shared_ptr<T>> children;
    public:
        ListNode() {}
        ListNode(const std::shared_ptr<T> &val) { children.push_back(val); }
        ListNode(std::shared_ptr<T> &&val) { children.push_back(val); }
        ~ListNode() {}

        std::list<std::shared_ptr<T>> &getChildren() { return children; }

        void append(const std::shared_ptr<T> &val) { children.push_back(val); }
        void append(std::shared_ptr<T> &&val) { children.push_back(val); }

        void merge(const std::shared_ptr<ListNode<T>> &rhs)
        {
            for (auto &c: rhs->children)
            {
                children.push_back(c);
            }
            // children.merge(std::move(rhs->children));
        }
        void merge(std::shared_ptr<ListNode<T>> &&rhs)
        {
            children.merge(std::move(rhs->children));
        }
        void mergeList(const std::list<std::shared_ptr<T>> &lst)
        {
            for (auto &c: lst)
            {
                children.push_back(c);
            }
            // children.merge(std::move(lst));
        }
        void mergeList(std::list<std::shared_ptr<T>> &&lst)
        {
            children.merge(std::move(lst));
        }

        //代码生成 对list里的每个元素调用codegen
        virtual llvm::Value *codegen(CodegenContext &context)
        {
            for (auto &c : children)
            {
                c->codegen(context);
            }
        }
    };


    class ExprNode: public BasicNode
    {
    public:
        ExprNode() {}
        ~ExprNode() {}
        virtual llvm::Value *codegen(CodegenContext &context) = 0;
    };

    class LeftExprNode: public ExprNode
    {
    public:
        LeftExprNode() {}
        ~LeftExprNode() = default;
        virtual llvm::Value *codegen(CodegenContext &context) = 0;
        virtual llvm::Value *getPtr(CodegenContext &context) = 0;
        virtual llvm::Value *getAssignPtr(CodegenContext &context) = 0;
        virtual const std::string getSymbolName() = 0;
    };
    
    class StmtNode: public BasicNode
    {
    public:
        StmtNode() {}
        ~StmtNode() {}
        virtual llvm::Value *codegen(CodegenContext &context) = 0;
    };
    
    //add new node
    template<typename T, typename... Args>
    std::shared_ptr<T> make_new_node(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    class IdNode: public LeftExprNode
    {      
    public:
        std::string name;
        IdNode(const std::string &str)
            : name(str) 
        {
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            //换成小写 
        }
        IdNode(const char *str)
            : name(str) 
        {
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        }
        ~IdNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        llvm::Constant *getConstVal(CodegenContext &context);
        llvm::Value *getPtr(CodegenContext &context) override;
        llvm::Value *getAssignPtr(CodegenContext &context) override;
        const std::string getSymbolName() override { return this->name; }
    };

    using IdentifierList = ListNode<IdNode>;

    enum Type { Unknown, Void, Int, Real, String, Array, Record, Bool, Long, Char, Alias };

    class TypeNode: public BasicNode
    {
    public:
        Type type;
        TypeNode(const Type type = Unknown) : type(type) {}
        ~TypeNode() {}
        llvm::Value *codegen(CodegenContext &) override { return nullptr; };
        virtual llvm::Type *findType(CodegenContext &) = 0;
    };

    class VoidTypeNode: public TypeNode
    {
    public:
        VoidTypeNode() : TypeNode(Type::Void) {}
        ~VoidTypeNode() = default;
        llvm::Type *findType(CodegenContext &context) override ;
    };
    
    class SimpleTypeNode: public TypeNode
    {
    public:
        SimpleTypeNode(const Type type) : TypeNode(type) {}
        ~SimpleTypeNode() = default;
        llvm::Type *findType(CodegenContext &) override;
    };
    
    class StringTypeNode: public TypeNode
    {
    public:
        StringTypeNode() : TypeNode(Type::String) {}
        ~StringTypeNode() = default;
        llvm::Type *findType(CodegenContext &context) override;
    };

    class AliasTypeNode: public TypeNode
    {
    public:
        std::shared_ptr<IdNode> name;
        AliasTypeNode(const std::shared_ptr<IdNode> &name)
            : TypeNode(Type::Alias), name(name) {}
        ~AliasTypeNode() = default;
        llvm::Type *findType(CodegenContext &context) override;
    };

    class VarDeclNode;
    
    // TODO: RecordTypeNode
    class RecordTypeNode: public TypeNode
    {
        
    private:
        std::list<std::shared_ptr<VarDeclNode>> field;
    public:
        RecordTypeNode(const std::shared_ptr<IdentifierList> &names, const std::shared_ptr<TypeNode> &type)
            : TypeNode(Type::Record)
        {
            for (auto &id : names->getChildren())
            {
                field.push_back(make_new_node<VarDeclNode>(id, type));
            }
        }
        ~RecordTypeNode() = default;
        
        void append(const std::shared_ptr<VarDeclNode> &var);
        void merge(const std::shared_ptr<RecordTypeNode> &rhs);
        void merge(std::shared_ptr<RecordTypeNode> &&rhs);
        llvm::Type *findType(CodegenContext &context) override;
        llvm::Value *getFieldIdx(const std::string &name, CodegenContext &context);
        void insertNestedRecord(const std::string &outer, CodegenContext &context);
        friend class CodegenContext;
    };
    

    class ConstValueNode: public ExprNode
    {
    public:
        Type type;
        ConstValueNode(const Type type = Type::Unknown): type(type) {}
        ~ConstValueNode() = default;

        llvm::Type *findType(CodegenContext &context);
    };
    
    class BooleanNode: public ConstValueNode
    {
    public:
        bool val;
        BooleanNode(const bool val = false): ConstValueNode(Type::Bool), val(val) {}
        ~BooleanNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
    };

    class IntegerNode: public ConstValueNode
    {
    public:
        int val;
        IntegerNode(const int val = 0): ConstValueNode(Type::Int), val(val) {}
        ~IntegerNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
    };

    class RealNode: public ConstValueNode
    {
    public:
        double val;
        RealNode(const double val = 0.0): ConstValueNode(Type::Real), val(val) {}
        ~RealNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
    };

    class CharNode: public ConstValueNode
    {
    public:
        char val;
        CharNode(const char val = '\0'): ConstValueNode(Type::Char), val(val) {}
        ~CharNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
    };

    class StringNode: public ConstValueNode
    {
    public:
        std::string val;
        StringNode(const char *val = ""): ConstValueNode(Type::String), val(val) {}
        StringNode(const std::string &val): ConstValueNode(Type::String), val(val) {}
        ~StringNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
    };
    
    class ArrayTypeNode: public TypeNode
    {
    public:
        std::shared_ptr<ExprNode> range_start;
        std::shared_ptr<ExprNode> range_end;
        std::shared_ptr<TypeNode> itemType;

        ArrayTypeNode(
            const std::shared_ptr<ExprNode> &start,
            const std::shared_ptr<ExprNode> &end,
            const std::shared_ptr<TypeNode> &itype
        ) : TypeNode(Type::Array), range_start(start), range_end(end), itemType(itype) {}
        ArrayTypeNode(
            int start,
            int end,
            Type itype
        ) : TypeNode(Type::Array), 
            range_start(cast_node<ExprNode>(make_new_node<IntegerNode>(start))), range_end(cast_node<ExprNode>(make_new_node<IntegerNode>(end))),
            itemType(make_new_node<SimpleTypeNode>(itype))
        {}
        ~ArrayTypeNode() = default;
        llvm::Type *findType(CodegenContext &) override;
        void insertNestedArray(const std::string &outer, CodegenContext &context);
    };
    
     //各种类型声明的节点
    class DeclNode: public BasicNode
    {
    public:
        DeclNode() {}
        ~DeclNode() = default;
    };
    
    class VarDeclNode: public DeclNode
    {
    private:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<TypeNode> type;
    public:
        VarDeclNode(const std::shared_ptr<IdNode>& name, const std::shared_ptr<TypeNode>& type) : name(name), type(type) {}
        ~VarDeclNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
        friend class RecordTypeNode;
        llvm::Value *createGlobalArray( CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &);
        llvm::Value *createArray(CodegenContext &context, const std::shared_ptr<ArrayTypeNode> &);
        friend class CodegenContext;
    };

    class ConstDeclNode: public DeclNode
    {
    private:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<ConstValueNode> val;
    public:
        ConstDeclNode(const std::shared_ptr<IdNode>& name, const std::shared_ptr<ConstValueNode>& val) : name(name), val(val) {}
        ~ConstDeclNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
    };
    
    class TypeDeclNode: public DeclNode
    {
    private:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<TypeNode> type;
    public:
        TypeDeclNode(const std::shared_ptr<IdNode>& name, const std::shared_ptr<TypeNode>& type) : name(name), type(type) {}
        ~TypeDeclNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
    };

    class ParamNode: public DeclNode
    {
    private:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<TypeNode> type;
    public:
        ParamNode(const std::shared_ptr<IdNode>& name, const std::shared_ptr<TypeNode>& type) : name(name), type(type) {}
        ~ParamNode() = default;

        llvm::Value *codegen(CodegenContext &) override { return nullptr; }
        friend class ASTvis;
        friend class RoutineNode;
    };
    
    using TypeDeclList = ListNode<TypeDeclNode>;
    using ConstDeclList = ListNode<ConstDeclNode>;
    using VarDeclList = ListNode<VarDeclNode>;
    using ArgList = ListNode<ExprNode>;
    using ParamList = ListNode<ParamNode>;

    // enum UnaryOp { Pos, Neg, Not };
    enum BinaryOp
    {
        Plus, Minus, Mul, Div, Mod, Truediv, And, Or, Xor,
        Eq, Neq, Gt, Lt, Geq, Leq
    };

    class BinaryExprNode: public ExprNode
    {
    private:
        BinaryOp op;
        std::shared_ptr<ExprNode> lhs, rhs;
    public:
        BinaryExprNode(
            const BinaryOp op, 
            const std::shared_ptr<ExprNode>& lval, 
            const std::shared_ptr<ExprNode>& rval
            ) 
            : op(op), lhs(lval), rhs(rval) {}
        ~BinaryExprNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
    };
    
    class ArrayRefNode: public LeftExprNode
    {
    private:
        std::shared_ptr<LeftExprNode> arr;
        std::shared_ptr<ExprNode> index;
    public:
        ArrayRefNode(const std::shared_ptr<LeftExprNode> &arr, const std::shared_ptr<ExprNode> &index)
            : arr(arr), index(index) {}
        ~ArrayRefNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        llvm::Value *getPtr(CodegenContext &) override;
        llvm::Value *getAssignPtr(CodegenContext &) override;
        const std::string getSymbolName() override;
        friend class ASTvis;
        friend class AssignStmtNode;
    };

    class RecordRefNode: public LeftExprNode
    {
    private:
        std::shared_ptr<LeftExprNode> name;
        std::shared_ptr<IdNode> field;
    public:
        RecordRefNode(const std::shared_ptr<LeftExprNode> &name, const std::shared_ptr<IdNode> &field)
            : name(name), field(field) {}
        ~RecordRefNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        llvm::Value *getPtr(CodegenContext &) override;
        llvm::Value *getAssignPtr(CodegenContext &) override;
        const std::string getSymbolName() override;
        friend class ASTvis;
    };

    class ProcNode: public ExprNode
    {
    public:
        ProcNode() = default;
        ~ProcNode() = default;
        llvm::Value *codegen(CodegenContext &context) = 0;
        // void print() = 0;
    };

    class CustomProcNode: public ProcNode
    {
    private:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<ArgList> args;
    public:
        CustomProcNode(const std::string &name, const std::shared_ptr<ArgList> &args = nullptr) 
            : name(make_new_node<IdNode>(name)), args(args) {}
        CustomProcNode(const std::shared_ptr<IdNode> &name, const std::shared_ptr<ArgList> &args = nullptr) 
            : name(name), args(args) {}
        ~CustomProcNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };

    enum SysFunc { Read, Readln, Write, Writeln, Abs, Chr, Odd, Ord, Pred, Sqr, Sqrt, Succ, Concat, Length, Str, Val };
     ;  
    class SysProcNode: public ProcNode
    {
    private:
        SysFunc name;
        std::shared_ptr<ArgList> args;
        std::string width;
    public:
        SysProcNode(const SysFunc name, const std::shared_ptr<ArgList> &args = nullptr,const std::string width = "") 
            : name(name), args(args) ,width(width){}
        ~SysProcNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };

    using CompoundStmtNode = ListNode<StmtNode>;

    class IfStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<ExprNode> expr;
        std::shared_ptr<CompoundStmtNode> if_stmt;
        std::shared_ptr<CompoundStmtNode> else_stmt;
    public:
        IfStmtNode(
            const std::shared_ptr<ExprNode> &expr, 
            const std::shared_ptr<CompoundStmtNode> &if_stmt, 
            const std::shared_ptr<CompoundStmtNode> &else_stmt = nullptr
            ) 
            : expr(expr), if_stmt(if_stmt), else_stmt(else_stmt) {}
        ~IfStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };
    
    class WhileStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<ExprNode> expr;
        std::shared_ptr<CompoundStmtNode> stmt;
    public:
        WhileStmtNode(
            const std::shared_ptr<ExprNode> &expr, 
            const std::shared_ptr<CompoundStmtNode> &stmt
            )
            : expr(expr), stmt(stmt) {}
        ~WhileStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };

    
    enum ForDirection { To, Downto };
    
    class ForStmtNode: public StmtNode
    {
    private:
        ForDirection direction;
        std::shared_ptr<IdNode> id;
        std::shared_ptr<ExprNode> init_val;
        std::shared_ptr<ExprNode> end_val;
        std::shared_ptr<CompoundStmtNode> stmt;
    public:
        ForStmtNode(
            const ForDirection dir,
            const std::shared_ptr<IdNode> &id, 
            const std::shared_ptr<ExprNode> &init_val, 
            const std::shared_ptr<ExprNode> &end_val, 
            const std::shared_ptr<CompoundStmtNode> &stmt
            )
            : direction(dir), id(id), init_val(init_val), end_val(end_val), stmt(stmt) {}
        ~ForStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };
    
    class RepeatStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<ExprNode> expr;
        std::shared_ptr<CompoundStmtNode> stmt;
        std::shared_ptr<IdNode> id;
    public:
        RepeatStmtNode(
            const std::shared_ptr<ExprNode> &expr, 
            const std::shared_ptr<CompoundStmtNode> &stmt,
            const std::shared_ptr<IdNode> &id
            )
            : expr(expr), stmt(stmt) ,id(id){}
        ~RepeatStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };

    class ProcStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<ProcNode> call;
    public:
        ProcStmtNode(const std::shared_ptr<ProcNode> &call) : call(call) {}
        ~ProcStmtNode() = default;
        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };

    class AssignStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<LeftExprNode> lhs;
        std::shared_ptr<ExprNode> rhs;
    public:
        AssignStmtNode(const std::shared_ptr<LeftExprNode> &lhs, const std::shared_ptr<ExprNode> &rhs)
            : lhs(lhs), rhs(rhs)
        {}
        ~AssignStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };
    

    // TODO: Case
    class CaseBranchNode: public StmtNode
    {
    private:
        std::shared_ptr<ExprNode> branch;
        std::shared_ptr<CompoundStmtNode> stmt;
    public:
        CaseBranchNode(const std::shared_ptr<ExprNode> &branch, const std::shared_ptr<CompoundStmtNode> &stmt)
            : branch(branch), stmt(stmt) {}
        ~CaseBranchNode() = default;

        llvm::Value *codegen(CodegenContext &context) override { return nullptr; }
        friend class ASTvis;
        friend class SwitchStmtNode;
    };

    using CaseBranchList = ListNode<CaseBranchNode>;

    class SwitchStmtNode: public StmtNode
    {
    private:
        std::shared_ptr<ExprNode> expr;
        std::list<std::shared_ptr<CaseBranchNode>> branches;
    public:
        SwitchStmtNode(const std::shared_ptr<ExprNode> &expr, const std::shared_ptr<CaseBranchList> &list)
            : expr(expr), branches(list->getChildren()) {}
        SwitchStmtNode(const std::shared_ptr<ExprNode> &expr, std::shared_ptr<CaseBranchList> &&list)
            : expr(expr), branches(std::move(list->getChildren())) {}
        ~SwitchStmtNode() = default;

        llvm::Value *codegen(CodegenContext &context) override;
        friend class ASTvis;
    };
    
    class RoutineNode;
    using RoutineList = ListNode<RoutineNode>;

    class RoutineHeadNode: public BasicNode
    {
    private:
        std::shared_ptr<ConstDeclList> constList;
        std::shared_ptr<VarDeclList> varList;
        std::shared_ptr<TypeDeclList> typeList;
        std::shared_ptr<RoutineList> subroutineList;
    public:
        RoutineHeadNode(
            const std::shared_ptr<ConstDeclList> &constList,
            const std::shared_ptr<VarDeclList> &varList,
            const std::shared_ptr<TypeDeclList> &typeList,
            const std::shared_ptr<RoutineList> &subroutineList
            )
            : constList(constList), varList(varList), typeList(typeList), subroutineList(subroutineList) {}
        ~RoutineHeadNode() = default;

        llvm::Value *codegen(CodegenContext &) override { return nullptr; }
        friend class ASTvis;
        friend class ProgramNode;
        friend class RoutineNode;
    };

    class BaseRoutineNode: public BasicNode
    {
    protected:
        std::shared_ptr<IdNode> name;
        std::shared_ptr<InitNode> init_part;
        std::shared_ptr<RoutineHeadNode> routine_head;
        std::shared_ptr<CompoundStmtNode> routine_body;
    public:
        BaseRoutineNode(const std::shared_ptr<IdNode> &name,const std::shared_ptr<RoutineHeadNode> &routine_head, const std::shared_ptr<CompoundStmtNode> &routine_body, const std::shared_ptr<InitNode> &init_part = nullptr )
            : name(name), init_part(init_part), routine_head(routine_head), routine_body(routine_body) {}
        ~BaseRoutineNode() = default;

        std::string getName() const { return name->name; }
        llvm::Value *codegen(CodegenContext &) = 0;
        // void print() = 0;
        friend class ASTvis;
    };

    class RoutineNode: public BaseRoutineNode
    {
    private:
        std::shared_ptr<ParamList> params;
        std::shared_ptr<TypeNode> retType;
    public:
        RoutineNode(
            const std::shared_ptr<IdNode> &name, 
            const std::shared_ptr<RoutineHeadNode> &routine_head, 
            const std::shared_ptr<CompoundStmtNode> &routine_body, 
            const std::shared_ptr<ParamList> &params, 
            const std::shared_ptr<TypeNode> &retType
            )
            : BaseRoutineNode(name,  routine_head, routine_body,nullptr), params(params), retType(retType) {}
        ~RoutineNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
    };

    class ProgramNode: public BaseRoutineNode
    {
    public:
        using BaseRoutineNode::BaseRoutineNode;
        ~ProgramNode() = default;

        llvm::Value *codegen(CodegenContext &) override;
        friend class ASTvis;
    };   

} // namespace fpc


#endif

