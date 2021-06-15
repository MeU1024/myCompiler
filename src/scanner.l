%{
#include<iostream>
#include<stdio.h>
#include<string>
#include<stdexcept>
#include "parser.hpp"
#include "basis.hpp"

#undef YY_DECL
#define YY_DECL int yylex(fpc::parser::semantic_type* lval, fpc::parser::location_type* loc)
#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

using id = fpc::parser::token::yytokentype;
%}

EXP [^']
%option caseless
%option noyywrap

%%
%{
    fpc::parser::semantic_type* yylval = lval;
%}

"("     {return id::LP;}
")"     {return id::RP;}
"["     {return id::LB;}
"]"     {return id::RB;}
"."     {return id::DOT;}
".."    {return id::DOTDOT;}
";"     {return id::SEMI;}
","     {return id::COMMA;}
":"     {return id::COLON;}
"*"     {return id::MUL;}
"/"     {return id::TRUEDIV;}
"+"     {return id::PLUS;}
"-"     {return id::MINUS;}
">="    {return id::GE;}
">"     {return id::GT;}
"<="    {return id::LE;}
"<"     {return id::LT;}
"<>"    {return id::UNEQUAL;}
"="     {return id::EQUAL;}
":="    {return id::ASSIGN;}

"AND"       {  return id::AND;}
"ARRAY"     {  return id::ARRAY;}
"CONST"     {  return id::CONST;}
"COUNTER"   {  return id::COUNTER;}
"DIV"       {  return id::DIV;}
"MOD"       {  return id::MOD;}
"DO"        {  return id::DO;}
"DOWNTO"    {
      
    yylval->build<fpc::ForDirection>(fpc::ForDirection::Downto);
    return id::DOWNTO;
}
"ELSE"      {  return id::ELSE;}
"END"       {  return id::END;}
"FOR"       {  return id::FOR;}
"FUNCTION"  {  return id::FUNCTION;}
"GOTO"      {  return id::GOTO;}
"IF"        {  return id::IF;}
"INIT"      {  return id::INIT;}
"INITEND"   {  return id::INITEND;}
"NOT"       {  return id::NOT;}
"OF"        {  return id::OF;}
"OR"        {  return id::OR;}
"XOR"       {  return id::XOR;}
"BEGIN"     {  return id::PBEGIN;}
"PROCEDURE" {  return id::PROCEDURE;}
"PROGRAM"   {  return id::PROGRAM;}
"READLN"      {
      
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Readln);
    return id::SYS_PROC;
}
"READ"      {
      
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Read);
    return id::SYS_PROC;
}
"REPEATDO"    {  return id::REPEATDO;}
"THEN"      {  return id::THEN;}
"TO"        {
     
    yylval->build<fpc::ForDirection>(fpc::ForDirection::To); 
    return id::TO;
}
"TYPE"      {  return id::TYPE;}
"TIMES"     {  return id::TIMES;}
"VAR"       {  return id::VAR;}
"WHILE"     {  return id::WHILE;}
"RECORD"    {  return id::RECORD;}
"SWITCH"    {  return id::SWITCH;}

"FALSE"     {
     
    yylval->build<std::shared_ptr<ConstValueNode>>(make_new_node<BooleanNode>(false)); 
    return id::SYS_CON;
}
"MAXINT"    {
     
    yylval->build<std::shared_ptr<ConstValueNode>>(make_new_node<IntegerNode>(std::numeric_limits<int>::max()));
    return id::SYS_CON;
}
"TRUE"      {
    
    yylval->build<std::shared_ptr<ConstValueNode>>(make_new_node<BooleanNode>(true)); 
    return id::SYS_CON;
}
"ABS"       {
    
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Abs);
    return id::SYS_FUNCT;
}
"CHR"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Chr);
    return id::SYS_FUNCT;
}
"CONCAT"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Concat);
    return id::SYS_FUNCT;
}
"LENGTH"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Length);
    return id::SYS_FUNCT;
}
"ODD"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Odd);
    return id::SYS_FUNCT;
}
"ORD"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Ord);
    return id::SYS_FUNCT;
}
"PRED"      {
         
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Pred);
    return id::SYS_FUNCT;
}
"SQR"       {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Sqr);
    return id::SYS_FUNCT;
}
"SQRT"      {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Sqrt);
    return id::SYS_FUNCT;
}
"STR"      {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Str);
    return id::SYS_FUNCT;
}
"SUCC"      {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Succ);
    return id::SYS_FUNCT;
}
"VAL"     {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Val);
    return id::SYS_FUNCT;
}
"WRITE"     {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Write);
    return id::SYS_PROC;
}
"WRITELN"   {
     
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Writeln);
    return id::SYS_PROC;
}

"BOOLEAN"   {
     
    yylval->build<std::shared_ptr<SimpleTypeNode>>(make_new_node<SimpleTypeNode>(fpc::Type::Bool));
    return id::SYS_TYPE;
}
"CHAR"      {
     
    yylval->build<std::shared_ptr<SimpleTypeNode>>(make_new_node<SimpleTypeNode>(fpc::Type::Char));  
    return id::SYS_TYPE;
}
"INTEGER"   {
     
    yylval->build<std::shared_ptr<SimpleTypeNode>>(make_new_node<SimpleTypeNode>(fpc::Type::Int)); 
    return id::SYS_TYPE;
}
"LONGINT"   {
     
    yylval->build<std::shared_ptr<SimpleTypeNode>>(make_new_node<SimpleTypeNode>(fpc::Type::Long)); 
    return id::SYS_TYPE;
}
"REAL"      {
    
    yylval->build<std::shared_ptr<SimpleTypeNode>>(make_new_node<SimpleTypeNode>(fpc::Type::Real)); 
    return id::SYS_TYPE;
}
"STRING"    {
    
    return id::STR_TYPE;
}

[+-]?[0-9]+      {
    yylval->build<std::shared_ptr<IntegerNode>>(make_new_node<IntegerNode>(atoi(yytext))); 
    return id::INTEGER;
}
[0-9]+\.[0-9]+   {
    yylval->build<std::shared_ptr<RealNode>>(make_new_node<RealNode>(atof(yytext))); 
    return id::REAL;
}
'{EXP}'  {
    yylval->build<std::shared_ptr<CharNode>>(make_new_node<CharNode>(yytext[1])); 
    return id::CHAR;
}
'({EXP}|'')+'  {
    yytext[yyleng-1] = 0; 
    yylval->build<std::shared_ptr<StringNode>>(make_new_node<StringNode>(yytext + 1)); 
    return id::STRING;
}
[a-zA-Z_]([a-zA-Z0-9_])*  {
   
    yytext[yyleng] = 0;
    yylval->build<std::shared_ptr<IdNode>>(make_new_node<IdNode>(yytext));  
    return id::ID;
}
[ \t\f]    { continue;}
[\n\r]     { continue;}

"//" {
    char c;
    while(c = yyinput()) 
    {
        if(c == '\n' | c== EOF) 
        {
            break;
        }
    }
}

. {
    
    throw std::invalid_argument(std::string("Invalid token \'") + yytext + "\'");
}

%%

