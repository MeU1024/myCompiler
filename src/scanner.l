%{
#include<iostream>
#include<stdio.h>
#include<string>    
#include "parser.tab.hpp"

#undef YY_DECL
#define YY_DECL int yylex(fpc::parser::semantic_type* lval, fpc::parser::location_type* loc)

using id = fpc::parser::token::yytokentype;
%}

NQUOTE [^']
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
"CASE"      {  return id::CASE;}
"CONST"     {  return id::CONST;}
"DIV"       {  return id::DIV;}
"MOD"       {  return id::MOD;}
"DO"        {  return id::DO;}
"DOWNTO"    {  return id::DOWNTO;}
"ELSE"      {  return id::ELSE;}
"END"       {  return id::END;}
"FOR"       {  return id::FOR;}
"FUNCTION"  {  return id::FUNCTION;}
"GOTO"      {  return id::GOTO;}
"IF"        {  return id::IF;}
"NOT"       {  return id::NOT;}
"OF"        {  return id::OF;}
"OR"        {  return id::OR;}
"XOR"       {  return id::XOR;}
"BEGIN"     {  return id::PBEGIN;}
"PROCEDURE" {  return id::PROCEDURE;}
"PROGRAM"   {  return id::PROGRAM;}
"READLN"      {
    /* std::cout << yytext; */  
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Readln);
    return id::SYS_PROC;
}
"READ"      {
    /* std::cout << yytext; */  
    yylval->build<fpc::SysFunc>(fpc::SysFunc::Read);
    return id::SYS_PROC;
}
"REPEAT"    {  return id::REPEAT;}
"THEN"      {  return id::THEN;}
"TO"        {  return id::TO;}
"TYPE"      {  return id::TYPE;}
"UNTIL"     {  return id::UNTIL;}
"VAR"       {  return id::VAR;}
"WHILE"     {  return id::WHILE;}
"RECORD"    { return id::RECORD;}

"FALSE"     { return id::SYS_CON;}
"MAXINT"    { return id::SYS_CON;}
"TRUE"      { return id::SYS_CON;}
"ABS"       { return id::SYS_FUNCT;}
"CHR"       { return id::SYS_FUNCT;}
"ODD"       { return id::SYS_FUNCT;}
"ORD"       { return id::SYS_FUNCT;}
"PRED"      { return id::SYS_FUNCT;}
"SQR"       { return id::SYS_FUNCT;}
"SQRT"      { return id::SYS_FUNCT;}
"SUCC"      { return id::SYS_FUNCT;}
"WRITE"     { return id::SYS_PROC;}
"WRITELN"   { return id::SYS_PROC;}

"BOOLEAN"   { return id::SYS_TYPE;}
"CHAR"      { return id::SYS_TYPE;}
"INTEGER"   { return id::SYS_TYPE;}
"REAL"      { return id::SYS_TYPE;}
"STRING"    { return id::STRING;}
"INIT"      { return id::INIT;}
"INITEND"   { return id::INITEND;}

[0-9]+      {std::cout << "Integer: " << yytext; return id::INTEGER;}
[0-9]+"."[0-9]+   {std::cout << "Real Number: " << yytext; return id::REAL;}
'{NQUOTE}'  {std::cout << "CHAR: " << yytext; return id::CHAR;}
'({NQUOTE}|'')+'  {std::cout << "STRING: " << yytext; yytext[yyleng-1] = 0; return id::STRING;}
[a-zA-Z_]([a-zA-Z0-9_])*  {std::cout << "IDD: " << yytext << " "; yytext[yyleng] = 0; return id::ID;}
[\t\f]    {std::cout << ' '; continue;}
[\n\r]     {std::cout << std::endl;}

"(*" {
    char c;
    while(c = yyinput()) 
    {
        if(c == '*') 
        {
            if((c = yyinput()) == ')')
                break;
            else unput(c);
        }
    }
}
"{" {
    char c;
    while(c = yyinput()) 
    {
        if(c == '}') break;
    }
}
"//" {
    char c;
    while(c = yyinput()) 
    {
        if(c == '\n') 
        {
            break;
        }
        else if(c == EOF) {
            break;
        }
    }
}

. {}

%%

