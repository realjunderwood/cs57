### TO RUN:

1. yacc -d parser.y
2. lex grammar.lex
3. g++ lex.yy.c y.tab.c semanticcheck.cpp ast.c -o compiler
4. ./compiler [minic file to compile]
