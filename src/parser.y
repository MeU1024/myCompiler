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

//TODO:
program: program_head routine DOT{
        printf("program: program_head routine DOT\n");
    }
    ;
//TODO:
program_head: PROGRAM ID SEMI{
        printf("program_head: PROGRAM ID SEMI\n");
    }
    ;
//TODO:
routine: routine_head initial routine_body {
        printf("routine: routine_head routine_body\n");
    }
    ;
//TODO:
routine_head: const_part type_part var_part routine_part  {
        printf("routine: const_part type_part var_part routine_part\n");
    }
    ;
//TODO:
initial:INIT  initial_body  INITEND{
        printf("inital: INIT  initial_body  INITEND\n");
    }
    ;
//TODO:
initial_body: initial_body proc_stmt SEMI{
        printf("initial_body : initial_body stmt_list\n");
    }
    | {printf("initial_body : empty\n");}
    ;


//TODO:
const_value: INTEGER {printf("const_value: INTEGER\n");}
    | REAL    {printf("const_value: REAL\n");}
    | CHAR    {printf("const_value: CHAR\n");}
    | STRING  {printf("const_value: STRING\n");}
    | SYS_CON {printf("const_value: SYS_CON\n");}
    ;


//TODO:
type_decl: simple_type_decl{
        printf("type_decl: simple_type_decl\n");
    }
    | array_type_decl {printf("type_decl: array_type_decl\n");}
    | record_type_decl {printf("type_decl: record_type_decl\n");}
    ;

//TODO:
simple_type_decl: SYS_TYPE {printf("simple_type_decl: SYS_TYPE\n");}
    | ID {printf("simple_type_decl: SYS_TYPE\n");}
    | LP name_list RP {printf("simple_type_decl: LP name_list RP\n");}
    ;

array_type_decl: ARRAY LB array_range RB OF type_decl {
        printf("array_type_decl: ARRAY LB array_range RB OF type_decl\n");
    }
    ;
//TODO:
array_range: const_value DOTDOT const_value
        { printf("array_range: const_value DOTDOT const_value\n"); }
    | ID DOTDOT ID
        { printf("array_range: ID DOTDOT ID\n"); }
    ;

//TODO:
name_list: name_list COMMA ID {
        printf("name_list: name_list COMMA ID\n");
    }
    | ID {printf("name_list: ID\n");}
    ;
//TODO:
var_part: VAR var_decl_list {printf("var_part: VAR var_decl_list\n");}
    | {printf("var_part: empty\n");}
    ;
//TODO:
var_decl_list: var_decl_list var_decl {
        printf("var_decl_list: var_decl_list var_decl\n");
    }
    | var_decl {printf("var_decl_list: var_decl\n");}
    ;
//TODO:
var_decl: name_list COLON type_decl SEMI {
        printf("var_decl: name_list COLON type_decl SEMI\n");
    }
    ;
//TODO:
routine_part: routine_part function_decl { 
        printf("routine_part: routine_part function_decl\n");}
    | routine_part procedure_decl {printf("routine_part: routine_part procedure_decl\n");}
    | {printf("routine_part: empty\n");}
    ;


//TODO:
routine_body: compound_stmt {
        printf("routine_body: compound_stmt\n");
    }
    ;
//TODO:
compound_stmt: PBEGIN stmt_list END {
        printf("compound_stmt: PBEGIN stmt_list END\n");
    }
    ;
//TODO:
stmt_list: stmt_list stmt SEMI {
        printf("stmt_list: stmt_list stmt SEMI\n");
    }
    | {printf("stmt_list: empty\n");}
    ;
//TODO:
stmt: assign_stmt {printf("stmt: assign_stmt\n");}
    | proc_stmt {printf("stmt_list: proc_stmt\n");}
    | compound_stmt {printf("stmt_list: compound_stmt\n");}
    | if_stmt {printf("stmt_list: if_stmt\n");}
    | repeat_stmt {printf("stmt_list: repeat_stmt\n");}
    | while_stmt {printf("stmt_list: while_stmt\n");}
    | for_stmt {printf("stmt_list: for_stmt\n");}
    | case_stmt {printf("stmt_list: case_stmt\n");}
    | goto_stmt {printf("stmt_list: goto_stmt\n");}
    ;
//TODO:
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
//TODO:
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


//TODO:
for_stmt: FOR ID ASSIGN expression direction expression DO stmt {
        printf("for_stmt: FOR ID ASSIGN expression direction expression DO stmt\n");
    }
    ;


//TODO:
expression: expression GE expr { printf("expression: expression GE expr\n"); }
    | expression GT expr { printf("expression: expression GT expr\n"); }
    | expression LE expr { printf("expression: expression LE expr\n"); }
    | expression LT expr { printf("expression: expression LT expr\n"); }
    | expression EQUAL expr { printf("expression: expression EQUAL expr\n"); }
    | expression UNEQUAL expr { printf("expression: expression UNEQUAL expr\n"); }
    | expr { printf("expression: expr\n"); }
    ;
//TODO:
expr: expr PLUS term { printf("expr: expr PLUS term\n"); }
    | expr MINUS term { printf("expr: expr MINUS term\n"); }
    | expr OR term { printf("expr: expr OR term\n"); }
    | expr XOR term { printf("expr: expr XOR term\n"); }
    | term { printf("expr: term\n"); }
    ;
//TODO:
term: term MUL factor { printf("term: term MUL factor\n"); }
    | term DIV factor { printf("term: term DIV factor\n"); }
    | term MOD factor { printf("term: term MOD factor\n"); }
    | term AND factor { printf("term: term AND factor\n"); }
    | term TRUEDIV factor { printf("term: term TRUEDIV factor\n");  }
    | factor { printf("term: factor\n"); }
    ;
//TODO:
factor: ID { printf("factor: ID\n"); }
    | ID LP args_list RP
        { printf("factor: ID LP args_list RP\n"); }
    | SYS_FUNCT LP args_list RP
        { printf("factor: SYS_FUNCT LP args_list RP\n"); }
    | const_value { printf("factor: const_value \n"); }
    | LP expression RP { printf("factor: LP expression RP\n"); }
    | NOT factor
        { printf("factor: NOT factor\n"); }
    | MINUS factor
        { printf("factor: MINUS factor\n"); }
    | ID LB expression RB
        { printf("factor: ID LB expression RB\n"); }
    | ID DOT ID
        { printf("factor: ID DOT ID\n"); }
    ;

args_list: args_list COMMA expression {
        printf("args_list: args_list COMMA expression\n");
    }
    | expression {
        printf("args_list: expression\n");
    }
    ;


%%
    
void fpc::parser::error(const fpc::parser::location_type &loc, const std::string& msg) {
    std::cerr << loc << ": " << msg << std::endl;
    throw std::logic_error("invalid syntax");
}
    
