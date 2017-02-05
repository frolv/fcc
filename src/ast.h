/*
 * ast.h
 */

#ifndef AST_H
#define AST_H

enum {
	NODE_VALUE,
	NODE_IDENTIFIER
};

enum {
	EXPR_COMMA
};

struct ast_node {
	int tag;
	union {
		long value;
		char *lexeme;
		unsigned long expr_flags;
	};
	struct symbol *sym;

	struct ast_node *left;
	struct ast_node *right;
};

struct ast_node *create_node(int tag, char *lexeme);
struct ast_node *create_expr(int tag, unsigned long flags,
                             struct ast_node *l, struct ast_node *r);

void free_tree(struct ast_node *root);

int ast_decl_set_type(struct ast_node *root, unsigned int flags);

void print_ast(FILE *f, struct ast_node *root);

#endif /* AST_H */
