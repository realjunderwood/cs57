#include "ast.h"
#include <vector>
#include <set>
#include <string>
#include <cstdio>

using namespace std;

vector<set<string>> scopeStack;
int error_count = 0;

void traverseStatement(astStmt* statement);

void traverse(astNode* node) {
	if (node == NULL) {
		return;
	}

	if (node->type == ast_prog) {
		traverse(node->prog.ext1);
		traverse(node->prog.ext2);
		traverse(node->prog.func);
	} else if (node->type == ast_func) {
		scopeStack.push_back(std::set<std::string>());

		if (node->func.param != NULL) {
			traverse(node->func.param); 
		}

		traverse(node->func.body);
		scopeStack.pop_back();
	} else if (node->type == ast_stmt) {
		traverseStatement(&(node->stmt));
	} else if (node->type == ast_rexpr) {
		traverse(node->rexpr.lhs);
		traverse(node->rexpr.rhs);
	} else if (node->type == ast_bexpr) {
		traverse(node->bexpr.lhs);
		traverse(node->bexpr.rhs);
	} else if (node->type == ast_uexpr) {
		traverse(node->uexpr.expr);
	} else if (node->type == ast_var) {
		string varName = node->var.name;
		bool found = false;
		for (int i = scopeStack.size() - 1; i >= 0; i--) {
			if (scopeStack[i].find(varName) != scopeStack[i].end()) {
				found = true;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "Error: variable used before declaration\n");
			error_count++;
		}
	}
}

void traverseStatement(astStmt* statement) {
	if (statement->type == ast_asgn) {
		traverse(statement->asgn.lhs);
		traverse(statement->asgn.rhs);
	} else if (statement->type == ast_block) {
		scopeStack.push_back(std::set<std::string>());
		for (astNode* line : *(statement->block.stmt_list)) {
			traverse(line);
		}
		scopeStack.pop_back();
	} else if (statement->type == ast_ret) {
		traverse(statement->ret.expr);
	} else if (statement->type == ast_while) {
		traverse(statement->whilen.cond);
		traverse(statement->whilen.body);
	} else if (statement->type == ast_if) {
		traverse(statement->ifn.cond);
		traverse(statement->ifn.if_body);
		if (statement->ifn.else_body) {
			traverse(statement->ifn.else_body);
		}
	} else if (statement->type == ast_call) {
		if (statement->call.param) {
			traverse(statement->call.param);
		}
	} else if (statement->type == ast_decl) {
		string varName = statement->decl.name;
		auto& currentScope = scopeStack.back();

		if (currentScope.find(varName) != currentScope.end()) {
			fprintf(stderr, "Error: variable declared multiple times in same scope\n");
			error_count++;
		} else {
			currentScope.insert(varName);
		}
	}
}

int semantic_check(astNode *root) {
	error_count = 0;
	scopeStack.clear();
	traverse(root);
	if (error_count > 0) {
		return 1;
	} else {
		return 0;
	}
}
