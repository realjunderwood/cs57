%{
	#include <stdio.h>
	#include "ast.h"
	#include <vector>
	extern int yylex();
	extern int yylex_destroy();
	extern int yywrap();
	extern FILE *yyin;
	astNode *root;
	void yyerror(const char *s);
%}

%union {
	int ival;
	char *sval;
	astNode *ast_node;
	std::vector<astNode*> *vec;
}

%type <ast_node> program functiondefinition block statement assignment print ifstatement whilestatement expr arithmetic_expr relational_expr decl_statement atomic_operand returnstatement extern_read extern_print
%type <vec> decl_list stmt_list

%token <ival> NUM
%token <sval> ID
%token INT VOID IF ELSE WHILE RETURN PRINT READ EQUAL_TO NOT_EQUAL_TO LTE GTE EXTERN

%%

/* PROGRAM */

program:
	extern_read extern_print functiondefinition {
		root = createProg($2, $1, $3);
		$$ = root; } |
	extern_print extern_read functiondefinition {
		root = createProg($1, $2, $3);
		$$ = root; }

functiondefinition:
	INT ID '(' INT ID ')' block {
		$$ = createFunc($2, createDecl($5), $7); } |
	INT ID '(' ')' block {
		$$ = createFunc($2, NULL, $5); }


/* EXTERNS */

extern_read:
	EXTERN INT READ '(' ')' ';' {
		$$ = createExtern("read"); }

extern_print:
	EXTERN VOID PRINT '(' INT ')' ';' {
		$$ = createExtern("print"); }

/* BLOCK and STMT_LIST */

stmt_list:
	{
		$$ = new std::vector<astNode*>(); } |
	stmt_list statement {
		$1->push_back($2);
		$$ = $1; }

decl_list:
	{
		$$ = new std::vector<astNode*>(); } |
	decl_list decl_statement {
		$1->push_back($2);
		$$ = $1; }

/* STATEMENT and TYPES OF STATEMENTS */

statement: /*note: this does not include decls*/
	ifstatement {
		$$ = $1; } |
	whilestatement {
		$$ = $1; } |
	block {
		$$ = $1; } |
	assignment {
		$$ = $1; } |
	print {
		$$ = $1; } |
	returnstatement {
		$$ = $1; }

ifstatement:
	IF '(' expr ')' statement {
		$$ = createIf($3,$5); } |
	IF '(' expr ')' statement ELSE statement {
		$$ = createIf($3,$5,$7); }

whilestatement:
	WHILE '(' expr ')' statement {
		$$ = createWhile($3, $5); }

returnstatement:
	RETURN expr ';' {
		$$ = createRet($2); }

block:
	'{' decl_list stmt_list '}' {
		std::vector<astNode*> *decls = $2;
		std::vector<astNode*> *stmts = $3;
		decls->insert(decls->end(), stmts->begin(), stmts->end());
		$$ = createBlock(decls);
		delete stmts; }

assignment:
	ID '=' expr ';' {
		$$ = createAsgn(createVar($1), $3); }

print:
	PRINT '(' expr ')' ';' {
		$$ = createCall("print", $3); }


decl_statement:
	INT ID ';' {
		$$ = createDecl($2); }

/* EXPRESSIONS */

expr:
	relational_expr {
		$$ = $1; } |
	arithmetic_expr {
		$$ = $1; }

relational_expr:
	arithmetic_expr EQUAL_TO arithmetic_expr {
		$$ = createRExpr($1, $3, eq); } |
	arithmetic_expr NOT_EQUAL_TO arithmetic_expr {
		$$ = createRExpr($1, $3, neq); } |
	arithmetic_expr '>' arithmetic_expr {
		$$ = createRExpr($1, $3, gt); } |
	arithmetic_expr '<' arithmetic_expr {
		$$ = createRExpr($1, $3, lt); } |
	arithmetic_expr GTE arithmetic_expr {
		$$ = createRExpr($1, $3, ge); } |
	arithmetic_expr LTE arithmetic_expr {
		$$ = createRExpr($1, $3, le); }


arithmetic_expr:
	atomic_operand {
		$$ = $1; } |
	'-' atomic_operand {
		$$ = createBExpr(createCnst(0),$2,sub); } |
	atomic_operand '+' atomic_operand {
		$$ = createBExpr($1,$3,add); } |
	atomic_operand '-' atomic_operand {
		$$ = createBExpr($1,$3,sub); } |
	atomic_operand '*' atomic_operand {
		$$ = createBExpr($1,$3,mul); } |
	atomic_operand '/' atomic_operand {
		$$ = createBExpr($1,$3,divide); }

/* atomic_operand AKA term */
atomic_operand:
	NUM {
		$$ = createCnst($1); } |
	ID {
		$$ = createVar($1); } |
	READ '(' ')' { $$ = createCall("read", NULL); } | /* spec is unclear on this, but p3 and p5 both have parentheses in return statement */
	'(' atomic_operand ')' {
		$$ = $2; }

%%

void yyerror(const char *s){
	fprintf(stderr,"%s\n", s);
}

extern int semantic_check(astNode *root);

int main(int argc, char* argv[]) {
	if (argc > 1) yyin = fopen(argv[1], "r");
	if (yyparse() == 0) { // If parsing succeeded
		printf("Parsed successfully\n");
		int result = semantic_check(root);
		if (result == 0) {
			printf("Semantics tests passed\n");
			return 0;
		} else {
			printf("Semantics tests failed\n");
			return 1;
		}
	} else {
		printf("Syntax parsing failed\n");
		return 1; //syntax parsing failed
	}
}
