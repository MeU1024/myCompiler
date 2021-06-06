%{
    #include <iostream>
    #include <memory>
    #include <string>
    #include <stdexcept>
%}

%token PROGRAM ID CONST ARRAY VAR FUNCTION PROCEDURE PBEGIN END TYPE RECORD
%token INTEGER REAL CHAR STRING
%token SYS_CON SYS_FUNCT SYS_PROC SYS_TYPE READ
%token IF THEN ELSE REPEAT UNTIL WHILE DO FOR TO DOWNTO CASE OF GOTO
%token ASSIGN EQUAL UNEQUAL LE LT GE GT
%token PLUS MINUS MUL DIV MOD TRUEDIV AND OR XOR NOT
%token DOT DOTDOT SEMI LP RP LB RB COMMA COLON

%type <std::string> program program_head
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