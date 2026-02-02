%{
	#include "ast.h"
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "y.tab.h"
%}

%%

"int" {
	return INT;
}
"void" {
	return VOID;
}
"return" {
	return RETURN;
}
"if" {
	return IF;
}
"else" {
	return ELSE;
}
"while" {
	return WHILE;
}
"print" {
	return PRINT;
}
"extern" {
	return EXTERN;
}
"read" {
	return READ;
}
"==" {
	return EQUAL_TO;
}
"!=" {
	return NOT_EQUAL_TO;
}
">=" {
	return GTE;
}
"<=" {
	return LTE;
}


[0-9]+ {
	yylval.ival = atoi(yytext);
	return NUM;
}

[A-Za-z_][A-Za-z0-9_]* {
	yylval.sval = strdup(yytext);
	return ID;
}

[ \t\n\,]

.	{
	return(yytext[0]);
}

%%

int yywrap(){
	return 1;
}
