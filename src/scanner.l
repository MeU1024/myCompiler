%{
#include<iostream>
#include<stdio.h>
#include<string>

%}

NQUOTE [^']
%option caseless
%option noyywrap

%%

"("     
")"     
"["     
"]"     
"."   
".."   
";"     
","     
":"     
"*"     
"/"     
"+"     
"-"    
">="    
">"     
"<="   
"<"    
"<>"    
"="     
":="    

"AND"       
"ARRAY"     
"CASE"      
"CONST"   
"DIV"     
"MOD"       
"DO"        
"DOWNTO"   
"ELSE"     
"END"      
"FOR"       
"FUNCTION"  
"GOTO"      
"IF"       
"NOT"      
"OF"        
"OR"       
"XOR"      
"BEGIN"     
"PROCEDURE" 
"PROGRAM"   
"READLN"     
"READ"     
"REPEAT"   
"THEN"      
"TO"       
"TYPE"     
"UNTIL"     
"VAR"       
"WHILE"     
"RECORD"    
"FALSE"     
"MAXINT"   
"TRUE"      
"ABS"      
"CHR"      
"CONCAT"      
"LENGTH"      
"ODD"       
"ORD"      
"PRED"     
"SQR"      
"SQRT"      
"STR"     
"SUCC"     
"VAL"    
"WRITE"    
"WRITELN"  
"BOOLEAN"  
"CHAR"     
"INTEGER"  
"LONGINT"   
"REAL"    
"STRING"  
[a-zA-Z_]([a-zA-Z0-9_])*  {
    /* std::cout << "IDD: " << yytext << " "; */
    yytext[yyleng] = 0;
    yylval->build<std::shared_ptr<IdentifierNode>>(make_node<IdentifierNode>(yytext));  
    return token::ID;
}
"(*" 
"{" 
"//" 
. 

%%

