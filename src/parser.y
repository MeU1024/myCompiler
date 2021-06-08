%skeleton "lalr1.cc"
%require "3.0"
%debug

// 声明命名空间与类名，结合使用 fpc::parser::
%define api.namespace {fpc}
// 使得类型与token定义可以使用各种复杂的结构与类型
%define api.value.type variant
// 开启断言功能
%define parse.assert

// 生成各种头文件
%defines
// 导入必要的头文件，定义命名空间
%code requires {
    #include <iostream>
    #include <memory>
    #include <string>
    #include <stdexcept>

    using namespace std;
    namespace fpc {}
    using namespace fpc;
    
}

%code {
    int yylex(fpc::parser::semantic_type* lval, fpc::parser::location_type* loc);
}

%locations
// 详细显示错误信息
%define parse.error verbose

// 定义terminal：token
%token PROGRAM ID CONST ARRAY VAR FUNCTION PROCEDURE PBEGIN END TYPE RECORD
%token INIT INITEND
%token INTEGER REAL CHAR STRING
%token SYS_CON SYS_FUNCT SYS_PROC SYS_TYPE READ
%token IF THEN ELSE REPEAT UNTIL WHILE DO FOR TO DOWNTO CASE OF GOTO
%token ASSIGN EQUAL UNEQUAL LE LT GE GT
%token PLUS MINUS MUL DIV MOD TRUEDIV AND OR XOR NOT
%token DOT DOTDOT SEMI LP RP LB RB COMMA COLON

%type <std::string> program program_head initial_body initial
%type <std::string> routine routine_head routine_body
%type <std::string> const_part type_part var_part routine_part
%type <std::string> const_expr_list const_value
%type <std::string> type_decl_list type_decl type_definition
%type <std::string> simple_type_decl
%type <std::string> array_type_decl array_range
%type <std::string> record_type_decl
%type <std::string> field_decl field_decl_list name_list
%type <std::string> var_para_list var_decl_list var_decl
%type <std::string> function_decl function_head
%type <std::string> procedure_head procedure_decl
%type <std::string> parameters para_decl_list para_type_list
%type <std::string> stmt_list stmt 
%type <std::string> assign_stmt proc_stmt compound_stmt 
%type <std::string> if_stmt else_clause
%type <std::string> repeat_stmt while_stmt goto_stmt
%type <std::string> for_stmt direction 
%type <std::string> case_stmt case_expr_list case_expr
%type <std::string> expression expr term factor
%type <std::string> args_list

%start program

%%

program: program_head initial routine DOT{
        printf("program: program_head routine DOT\n");
    }
    ;

program_head: PROGRAM ID SEMI{
        printf("program_head: PROGRAM ID SEMI\n");
    }
    ;

initial:INIT  initial_body  INITEND{
        printf("inital: INIT  initial_body  INITEND\n");
    }
    ;

initial_body: initial_body proc_stmt{
        printf("initial_body : initial_body proc_stmt\n");
    }
    | {printf("initial_body : empty\n");}
    ;

routine: routine_head routine_body {
        printf("routine: routine_head routine_body\n");
    }
    ;

routine_head: const_part type_part var_part routine_part {
        printf("routine: const_part type_part var_part routine_part\n");
    }
    ;

routine_body: compound_stmt {
        printf("routine_body: compound_stmt\n");
    }
    ;

compound_stmt: PBEGIN stmt_list END {
        printf("compound_stmt: PBEGIN stmt_list END\n");
    }
    ;

stmt_list: stmt_list stmt SEMI {
        printf("stmt_list: stmt_list stmt SEMI\n");
    }
    | {printf("stmt_list: empty\n");}
    ;

stmt: assign_stmt {printf("stmt: assign_stmt\n");}
    | proc_stmt {printf("stmt_list: proc_stmt\n");}
    | compound_stmt {printf("stmt_list: compound_stmt\n");}
    | for_stmt {printf("stmt_list: for_stmt\n");}
    ;

assign_stmt: ID ASSIGN expression {
        printf("assign_stmt: ID ASSIGN expression\n");
    }
    | ID LB expression RB ASSIGN expression {
        printf("assign_stmt: ID LB expression RB ASSIGN expression\n");
    }
    | ID DOT ID ASSIGN expression {
        printf("stmt_list: ID DOT ID ASSIGN expression\n");
    }
    ;

proc_stmt: ID { printf("proc_stmt: ID\n"); }
    | ID LP args_list RP
        { printf("proc_stmt: ID LP args_list RP\n"); }
    | SYS_PROC
        { printf("proc_stmt: SYS_PROC LP RP\n"); }
    | SYS_PROC LP args_list RP
        { printf("proc_stmt: SYS_PROC LP args_list RP\n"); }
    | READ LP factor RP
        { printf("proc_stmt: READ LP factor RP\n"); }
    ;

for_stmt: FOR ID ASSIGN expression direction expression DO stmt {
        printf("for_stmt: FOR ID ASSIGN expression direction expression DO stmt\n");
    }
    ;


%%
    
void fpc::parser::error(const fpc::parser::location_type &loc, const std::string& msg) {
    std::cerr << loc << ": " << msg << std::endl;
    throw std::logic_error("invalid syntax");
}
    
