/*
 * ast.h
 */

#ifndef FCC_AST_H
#define FCC_AST_H

#include "parse.h"

enum {
	NODE_CONSTANT,
	NODE_IDENTIFIER,
	NODE_NEWID,
	NODE_STRLIT,
	NODE_MEMBER,

	EXPR_COMMA,
	EXPR_ASSIGN,
	EXPR_LOGICAL_OR,
	EXPR_LOGICAL_AND,
	EXPR_OR,
	EXPR_XOR,
	EXPR_AND,
	EXPR_EQ,
	EXPR_NE,
	EXPR_LT,
	EXPR_GT,
	EXPR_LE,
	EXPR_GE,
	EXPR_LSHIFT,
	EXPR_RSHIFT,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MULT,
	EXPR_DIV,
	EXPR_MOD,
	EXPR_ADDRESS,
	EXPR_DEREFERENCE,
	EXPR_UNARY_PLUS,
	EXPR_UNARY_MINUS,
	EXPR_NOT,
	EXPR_LOGICAL_NOT,
	EXPR_FUNC,
	EXPR_MEMBER
};

#define TAG_IS_UNARY(tag) \
	((tag) >= EXPR_ADDRESS && (tag) <= EXPR_LOGICAL_NOT)

#define TAG_IS_COND(tag) \
	((tag) == EXPR_LOGICAL_OR || (tag) == EXPR_LOGICAL_AND || \
	 ((tag) >= EXPR_EQ && (tag) <= EXPR_GE) || (tag) == EXPR_LOGICAL_NOT)

/* A single node in the abstract syntax tree. */
struct ast_node {
	int tag;
	union {
		long value;
		char *lexeme;
	};
	struct type_information expr_flags;
	struct symbol *sym;

	struct ast_node *left;
	struct ast_node *right;
};

struct ast_node *create_node(int tag, char *lexeme);
struct ast_node *create_expr(int expr, struct ast_node *lhs, struct ast_node *rhs);

void free_tree(struct ast_node *root);

int ast_decl_set_type(struct ast_node *root, struct type_information *type);
int ast_cast(struct ast_node *expr, struct type_information *type);

#include <stdio.h>

void print_ast(FILE *f, struct ast_node *root);

#endif /* FCC_AST_H */
