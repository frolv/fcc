/*
 * ast.h
 */

#ifndef AST_H
#define AST_H

enum {
	NODE_CONSTANT,
	NODE_IDENTIFIER,
	NODE_STRLIT,

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
	EXPR_FUNC
};

struct ast_node {
	int tag;
	union {
		long value;
		char *lexeme;
	};
	unsigned int expr_flags;
	struct symbol *sym;

	struct ast_node *left;
	struct ast_node *right;
};

struct ast_node *create_node(int tag, char *lexeme);
struct ast_node *create_expr(int expr, struct ast_node *lhs, struct ast_node *rhs);

void free_tree(struct ast_node *root);

int ast_decl_set_type(struct ast_node *root, unsigned int flags);

void print_ast(FILE *f, struct ast_node *root);

#endif /* AST_H */
