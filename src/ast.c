/*
 * ast.c
 * Abstract syntax tree handling functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symtab.h"

static int char_const_val(char *lexeme);

/*
 * create_node:
 * Create a leaf AST node holding an ID, constant or string literal.
 */
struct ast_node *create_node(int tag, char *lexeme)
{
	struct ast_node *n;

	n = malloc(sizeof *n);
	n->tag = tag;

	switch (tag) {
	case NODE_IDENTIFIER:
		n->sym = symtab_entry(lexeme);
		n->lexeme = n->sym->id;
		n->expr_flags = n->sym->flags;
		n->left = NULL;
		n->right = NULL;
		break;
	case NODE_CONSTANT:
		n->sym = NULL;

		n->expr_flags = TYPE_INT;
		/* hex, octal and unsigned constants */
		if ((*lexeme == '0' && lexeme[1]) || strpbrk(lexeme, "uU"))
			n->expr_flags |= QUAL_UNSIGNED;

		if (*lexeme == '\'')
			n->value = char_const_val(lexeme);
		else
			n->value = strtol(lexeme, NULL, 0);

		n->left = NULL;
		n->right = NULL;
		break;
	case NODE_STRLIT:
		n->sym = NULL;
		n->lexeme = strdup(lexeme);
		n->expr_flags = TYPE_STRLIT;
		n->left = NULL;
		n->right = NULL;
		break;
	default:
		break;
	}

	return n;
}

static void check_expr_type(struct ast_node *expr);

/*
 * create_expr:
 * Create an AST node representing an expression
 * of type `expr` performed on `lhs` and `rhs`.
 */
struct ast_node *create_expr(int expr, struct ast_node *lhs, struct ast_node *rhs)
{
	struct ast_node *n;

	n = malloc(sizeof *n);
	n->tag = expr;
	n->sym = NULL;
	n->left = lhs;
	n->right = rhs;
	check_expr_type(n);

	return n;
}

/*
 * free_tree:
 * Free all nodes in the abstract syntax tree starting at root.
 */
void free_tree(struct ast_node *root)
{
	switch (root->tag) {
	case NODE_STRLIT:
		free(root->lexeme);
		break;
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
int ast_decl_set_type(struct ast_node *root, unsigned int type_flags)
{
	/*
	 * Variables can be declared without an explicit type,
	 * e.g. `unsigned i`, in which case the type is assumed
	 * to be int.
	 */
	if (!FLAGS_TYPE(type_flags))
		type_flags |= TYPE_INT;

	if (root->tag == NODE_IDENTIFIER) {
		if (root->sym->flags & 0xFF) {
			/* TODO: proper error handling function */
			fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
			        "%s has already been declared\n",
			        root->lexeme);
			return 1;
		}
		root->sym->flags |= type_flags;
		root->expr_flags = root->sym->flags;
	} else if (root->tag == EXPR_COMMA) {
		root->expr_flags = type_flags;
	}

	if (root->left) {
		if (ast_decl_set_type(root->left, type_flags) != 0)
			return 1;
	}
	if (root->right) {
		if (ast_decl_set_type(root->right, type_flags) != 0)
			return 1;
	}

	return 0;
}

/*
 * ast_cast:
 * Cast the expression `expr` to the type specified by `type_flags`.
 */
int ast_cast(struct ast_node *expr, unsigned int type_flags)
{
	int expr_type, expr_flags, flags_type;

	expr_flags = expr->expr_flags;
	expr_type = FLAGS_TYPE(expr_flags);
	flags_type = FLAGS_TYPE(type_flags);

	if (FLAGS_ISPTR(type_flags)) {
		/* A pointer type can be cast to any other pointer type. */
		if (FLAGS_ISPTR(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}

		/* An integer type can be cast to any pointer type. */
		if (expr_type == TYPE_INT || expr_type == TYPE_CHAR) {
			expr->expr_flags = type_flags;
			return 0;
		}
	} else if (flags_type == TYPE_INT || flags_type == TYPE_CHAR) {
		/* A pointer can be cast to an integer type. */
		if (FLAGS_ISPTR(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}

		/* An integer type can be cast to another integer type. */
		if (expr_type == TYPE_INT || expr_type == TYPE_CHAR) {
			expr->expr_flags = type_flags;
			return 0;
		}
	}

	return 1;
}

/* char_const_val: convert character constant string to integer value */
static int char_const_val(char *lexeme)
{
	if (lexeme[1] == '\\') {
		switch (lexeme[2]) {
		case 'n':
			return '\n';
		case 't':
			return '\t';
		case '\'':
			return '\'';
		case '"':
			return '"';
		case '\\':
			return '\\';
		case '0':
			return '\0';
		default:
			return 0;
		}
	} else {
		return lexeme[1];
	}
}

/*
 * check_assign_type:
 * Check if the LHS of an assignment statement is a valid lvalue
 * and that the RHS can be assigned to it.
 */
static void check_assign_type(struct ast_node *expr)
{
	expr->expr_flags = expr->left->expr_flags;
}

/*
 * check_address_type:
 * Set the type of an address-of expression
 * by increasing the level of indirection.
 */
static void check_address_type(struct ast_node *expr)
{
	unsigned int indirection;

	expr->expr_flags = expr->left->expr_flags;
	indirection = (expr->expr_flags >> 24) + 1;
	expr->expr_flags &= 0x00FFFFFF;
	expr->expr_flags |= indirection << 24;
}

/*
 * check_dereference_type:
 */
static void check_dereference_type(struct ast_node *expr)
{
	unsigned int indirection;

	if (!FLAGS_ISPTR(expr->left->expr_flags)) {
		fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
		        "cannot dereference non-pointer type\n");
		exit(1);
	}

	expr->expr_flags = expr->left->expr_flags;
	indirection = (expr->expr_flags >> 24) - 1;
	expr->expr_flags &= 0x00FFFFFF;
	expr->expr_flags |= indirection << 24;
}

static void check_func_type(struct ast_node *expr)
{
	/*
	 * If a function does not exist in the symbol table,
	 * its return type is assumed to be int.
	 */
	expr->expr_flags = TYPE_INT;
}

static void (*expr_type_func[])(struct ast_node *) = {
	[EXPR_ASSIGN] = check_assign_type,
	[EXPR_ADDRESS] = check_address_type,
	[EXPR_DEREFERENCE] = check_dereference_type,
	[EXPR_FUNC] = check_func_type
};

/*
 * check_expr_type:
 * Validate the types of expr's LHS and RHS and set type of `expr`.
 */
static void check_expr_type(struct ast_node *expr)
{
	switch (expr->tag) {
	case EXPR_COMMA:
		expr->expr_flags = expr->right->expr_flags;
		break;

	case EXPR_LOGICAL_OR:
	case EXPR_LOGICAL_AND:
	case EXPR_EQ:
	case EXPR_NE:
	case EXPR_LT:
	case EXPR_GT:
	case EXPR_LE:
	case EXPR_GE:
	case EXPR_LOGICAL_NOT:
		expr->expr_flags = TYPE_INT;
		break;

	default:
		if (expr_type_func[expr->tag])
			expr_type_func[expr->tag](expr);
		else
			expr->expr_flags = 0;
		break;
	}
}

static void print_type(FILE *f, unsigned int flags)
{
	int i;

	putc('[', f);
	if (flags & QUAL_UNSIGNED)
		fprintf(f, "unsigned ");
	if (FLAGS_TYPE(flags) == TYPE_INT)
		fprintf(f, "int");
	if (FLAGS_TYPE(flags) == TYPE_CHAR)
		fprintf(f, "char");
	if (FLAGS_TYPE(flags) == TYPE_VOID)
		fprintf(f, "void");
	if (FLAGS_TYPE(flags) == TYPE_STRLIT)
		fprintf(f, "const char[]");

	i = flags >> 24;
	if (i) {
		fputc(' ', f);
		while (i--)
			fputc('*', f);
	}

	fprintf(f, "]\n");
}

static char *expr_names[] = {
	[EXPR_COMMA]            = "COMMA",
	[EXPR_ASSIGN]           = "ASSIGN",
	[EXPR_LOGICAL_OR]       = "LOGICAL_OR",
	[EXPR_LOGICAL_AND]      = "LOGICAL_AND",
	[EXPR_OR]               = "OR",
	[EXPR_XOR]              = "XOR",
	[EXPR_AND]              = "AND",
	[EXPR_EQ]               = "EQUAL",
	[EXPR_NE]               = "NOT_EQUAL",
	[EXPR_LT]               = "LESS_THAN",
	[EXPR_GT]               = "GREATER_THAN",
	[EXPR_LE]               = "LESS_THAN/EQUAL",
	[EXPR_GE]               = "GREATER_THAN/EQUAL",
	[EXPR_ADD]              = "ADD",
	[EXPR_SUB]              = "SUBTRACT",
	[EXPR_LSHIFT]           = "LSHIFT",
	[EXPR_RSHIFT]           = "RSHIFT",
	[EXPR_MULT]             = "MULTIPLY",
	[EXPR_DIV]              = "DIVIDE",
	[EXPR_MOD]              = "MOD",
	[EXPR_ADDRESS]          = "ADDRESS-OF",
	[EXPR_DEREFERENCE]      = "DEREFERENCE",
	[EXPR_UNARY_PLUS]       = "UNARY_PLUS",
	[EXPR_UNARY_MINUS]      = "UNARY_MINUS",
	[EXPR_NOT]              = "NOT",
	[EXPR_LOGICAL_NOT]      = "LOGICAL_NOT",
	[EXPR_FUNC]             = "FUNCTION_CALL"
};

static void print_ast_depth(FILE *f, struct ast_node *root,
                            int depth, int cont, int line)
{
	int i;

	/* This is silly but it looks good. */
	if (depth) {
		for (i = 0; i < depth - 1; ++i)
			fprintf(f, "%s   ", line & (1 << i) ? "│" : " ");

		fprintf(f, "%s", cont ? "├" : "└");
		for (i = 0; i < 2; ++i)
			fprintf(f, "─");
		putc(' ', f);
	}

	if (f == stdout)
		fprintf(f, "\x1B[1;34m");

	switch (root->tag) {
	case NODE_IDENTIFIER:
		if (f == stdout)
			fprintf(f, "\x1B[0;37m");
		fprintf(f, "ID: %s ", root->lexeme);
		break;
	case NODE_CONSTANT:
		if (f == stdout)
			fprintf(f, "\x1B[0;37m");
		fprintf(f, "CONSTANT: %ld ", root->value);
		break;
	case NODE_STRLIT:
		if (f == stdout)
			fprintf(f, "\x1B[0;37m");
		fprintf(f, "STRLIT: %s ", root->lexeme);
		break;
	default:
		fprintf(f, "OP: %s ", expr_names[root->tag]);
		break;
	}
	print_type(f, root->expr_flags);

	if (f == stdout)
		fprintf(f, "\x1B[0;37m");

	if (root->left) {
		if (root->right) {
			cont = 1;
			line |= (!!root->left->left << depth);
		} else {
			cont = 0;
		}
		print_ast_depth(f, root->left, depth + 1, cont, line);
	}
	if (root->right) {
		line &= ~(1 << depth);
		print_ast_depth(f, root->right, depth + 1, 0, line);
	}
	if (!depth)
		putc('\n', f);
}

void print_ast(FILE *f, struct ast_node *root)
{
	print_ast_depth(f, root, 0, 0, 0);
}
