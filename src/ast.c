/*
 * ast.c
 * Abstract syntax tree handling functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symtab.h"

struct ast_node *create_node(int tag, char *lexeme)
{
	struct ast_node *n;

	n = malloc(sizeof *n);
	n->tag = tag;

	switch (tag) {
	case NODE_IDENTIFIER:
		n->sym = symtab_entry(lexeme);
		n->lexeme = n->sym->id;
		n->left = NULL;
		n->right = NULL;
		break;
	default:
		break;
	}

	return n;
}

struct ast_node *create_expr(int tag, unsigned long flags,
                             struct ast_node *l, struct ast_node *r)
{
	struct ast_node *n;

	n = malloc(sizeof *n);
	n->tag = tag;
	n->expr_flags = flags;
	n->sym = NULL;
	n->left = l;
	n->right = r;

	return n;
}

/*
 * free_tree:
 * Free all nodes in the abstract syntax tree starting at root.
 */
void free_tree(struct ast_node *root)
{
	switch (root->tag) {
	}

	if (root->left)
		free_tree(root->left);
	if (root->right)
		free_tree(root->right);
	free(root);
}

/*
 * ast_decl_set_type:
 * Set the types of all identifiers in AST declaration statement
 * starting at `root` to `flags`.
 */
int ast_decl_set_type(struct ast_node *root, unsigned int flags)
{
	if (root->tag == NODE_IDENTIFIER) {
		if (root->sym->flags & 0xFF) {
			/* TODO: proper error handling function */
			fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
			        "%s has already been declared\n",
			        root->lexeme);
			return 1;
		}

		root->sym->flags |= flags;
	}

	if (root->left) {
		if (ast_decl_set_type(root->left, flags) != 0)
			return 1;
	}
	if (root->right) {
		if (ast_decl_set_type(root->right, flags) != 0)
			return 1;
	}

	return 0;
}

static void print_id_type(FILE *f, struct ast_node *id)
{
	int i;

	putc('[', f);
	if (id->sym->flags & QUAL_UNSIGNED)
		fprintf(f, "unsigned ");
	if (id->sym->flags & TYPE_INT)
		fprintf(f, "int");
	if (id->sym->flags & TYPE_CHAR)
		fprintf(f, "char");

	i = id->sym->flags >> 24;
	if (i) {
		fputc(' ', f);
		while (i--)
			fputc('*', f);
	}

	fprintf(f, "]\n");
}

static void print_ast_depth(FILE *f, struct ast_node *root, int depth)
{
	int i;

	for (i = 0; i < depth; ++i)
		putc('\t', f);

	switch (root->tag) {
	case NODE_IDENTIFIER:
		fprintf(f, "ID: %s ", root->lexeme);
		print_id_type(f, root);
		break;
	case EXPR_COMMA:
		fprintf(f, "OP: COMMA\n");
		break;
	}

	if (root->left)
		print_ast_depth(f, root->left, depth + 1);
	if (root->right)
		print_ast_depth(f, root->right, depth + 1);
}

void print_ast(FILE *f, struct ast_node *root)
{
	print_ast_depth(f, root, 0);
}
