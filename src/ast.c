/*
 * ast.c
 * Abstract syntax tree handling functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "error.h"
#include "symtab.h"
#include "types.h"

static int char_const_val(char *lexeme);

/*
 * create_node:
 * Create a leaf AST node holding an ID, constant or string literal.
 */
struct ast_node *create_node(int tag, char *lexeme)
{
	struct ast_node *n;

	n = calloc(1, sizeof *n);
	n->tag = tag;

	switch (tag) {
	case NODE_IDENTIFIER:
		n->tag = NODE_IDENTIFIER;
		n->sym = symtab_entry(lexeme);
		if (!n->sym) {
			error_undeclared(lexeme);
			exit(1);
		}
		n->lexeme = strdup(n->sym->id);
		n->expr_flags = n->sym->flags;
		break;
	case NODE_NEWID:
		n->tag = NODE_IDENTIFIER;
		n->sym = symtab_entry_scope(lexeme);
		if (n->sym) {
			error_declared(lexeme);
			exit(1);
		}
		n->sym = symtab_add(lexeme, TYPE_INT);
		n->lexeme = strdup(n->sym->id);
		n->expr_flags = n->sym->flags;
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

		break;
	case NODE_STRLIT:
		n->lexeme = strdup(lexeme);
		n->expr_flags = TYPE_STRLIT;
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
	case NODE_IDENTIFIER:
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
		/* Can't declare a variable of type void. */
		if (FLAGS_TYPE(type_flags) == TYPE_VOID &&
		    !FLAGS_IS_PTR(root->sym->flags)) {
			fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
			        "%s declared as type `void'\n",
			        root->lexeme);
			return 1;
		}

		root->sym->flags = (root->sym->flags & 0xFF000000) | type_flags;
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
	int expr_flags;

	expr_flags = expr->expr_flags;

	if (!FLAGS_TYPE(type_flags))
		type_flags |= TYPE_INT;

	if (FLAGS_IS_PTR(type_flags)) {
		/* A pointer type can be cast to any other pointer type. */
		if (FLAGS_IS_PTR(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}

		/* An integer type can be cast to any pointer type. */
		if (FLAGS_IS_INTEGER(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}
	} else if (FLAGS_IS_INTEGER(type_flags)) {
		/* A pointer can be cast to an integer type. */
		if (FLAGS_IS_PTR(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}

		/* An integer type can be cast to another integer type. */
		if (FLAGS_IS_INTEGER(expr_flags)) {
			expr->expr_flags = type_flags;
			return 0;
		}
	} else if (FLAGS_TYPE(type_flags) == TYPE_VOID) {
		expr->expr_flags = type_flags;
		return 0;
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
 * is_lvalue:
 * Return 1 if the the expression tree starting at `expr`
 * represents an lvalue, or 0 otherwise.
 */
static int is_lvalue(struct ast_node *expr)
{
	return (expr->tag == NODE_IDENTIFIER && !FLAGS_IS_FUNC(expr->expr_flags))
		|| expr->tag == EXPR_DEREFERENCE;
}

/*
 * pointer_additive_scale:
 * Multiply operand by sizeof *ptr when performing
 * an additive operation on a pointer.
 */
static void pointer_additive_scale(struct ast_node *expr)
{
	struct ast_node **add, *tmp;
	size_t ptr_size;

	if (FLAGS_IS_PTR(expr->left->expr_flags)) {
		ptr_size = type_size(FLAGS_TYPE(expr->left->expr_flags));
		add = &expr->right;
	} else {
		ptr_size = type_size(FLAGS_TYPE(expr->right->expr_flags));
		add = &expr->left;
	}

	if (ptr_size == 1)
		return;

	if ((*add)->tag == NODE_CONSTANT) {
		(*add)->value *= ptr_size;
	} else {
		tmp = calloc(1, sizeof *tmp);
		tmp->tag = NODE_CONSTANT;
		tmp->expr_flags = TYPE_INT | QUAL_UNSIGNED;
		tmp->value = ptr_size;

		*add = create_expr(EXPR_MULT, *add, tmp);
	}
}

/*
 * check_assign_type:
 * Check if the LHS of an assignment statement is a valid lvalue
 * and that the RHS can be assigned to it.
 */
static void check_assign_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;
	const unsigned int void_star = TYPE_VOID | (1 << 24);

	lhs_flags = expr->left->expr_flags;
	rhs_flags = expr->right->expr_flags;

	if (!is_lvalue(expr->left)) {
		error_assign_type(expr->left);
		exit(1);
	}

	if (FLAGS_IS_PTR(lhs_flags)) {
		if (FLAGS_IS_PTR(rhs_flags)) {
			/*
			 * Different non `void *` pointer types can be assigned,
			 * to each other, but a warning should be issued.
			 */
			if (lhs_flags != rhs_flags &&
			    lhs_flags != void_star &&
			    rhs_flags != void_star)
				warning_imcompatible_ptr_assn(expr);

			expr->expr_flags = lhs_flags;
		} else if (FLAGS_IS_INTEGER(rhs_flags)) {
			/*
			 * Integer types can be assigned to pointers, but
			 * the value should be cast to indicate intent.
			 */
			warning_int_assign(expr);
			expr->expr_flags = lhs_flags;
		} else if (FLAGS_TYPE(lhs_flags) == TYPE_CHAR &&
		           FLAGS_INDIRECTION(lhs_flags) == 1 &&
		           FLAGS_TYPE(rhs_flags) == TYPE_STRLIT) {
			/* String literal can be assigned to `char *`. */
			expr->expr_flags = lhs_flags;
		} else {
			goto err_incompatible;
		}
		return;
	} else if (FLAGS_IS_PTR(rhs_flags)) {
		if (!FLAGS_IS_INTEGER(lhs_flags))
			goto err_incompatible;

		warning_ptr_assign(expr);
		expr->expr_flags = lhs_flags;
	}

	if (FLAGS_IS_INTEGER(lhs_flags) && FLAGS_IS_INTEGER(rhs_flags)) {
		expr->expr_flags = lhs_flags;
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * check_boolean_type:
 * Check that the operands of a boolean operator are compatible.
 */
static void check_boolean_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;

	lhs_flags = expr->left->expr_flags;

	if (FLAGS_IS_INTEGER(lhs_flags) || FLAGS_IS_PTR(lhs_flags)) {
		if (expr->right) {
			rhs_flags = expr->right->expr_flags;
			if (!FLAGS_IS_INTEGER(rhs_flags) &&
			    !FLAGS_IS_PTR(rhs_flags))
				goto err_incompatible;
		}
		expr->expr_flags = TYPE_INT;
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * integer_type_convert:
 * Perform type conversion in an operation between two integer types.
 * The less precise type gets converted to the more precise type.
 * If both types have the same precision and one of them is unsigned,
 * then the result of the operation is unsigned.
 */
static unsigned int integer_type_convert(int lhs_flags, int rhs_flags)
{
	unsigned int lhs_type, rhs_type;

	lhs_type = FLAGS_TYPE(lhs_flags);
	rhs_type = FLAGS_TYPE(rhs_flags);

	if (lhs_type == TYPE_CHAR && rhs_type == TYPE_CHAR) {
		return TYPE_CHAR | (lhs_flags & QUAL_UNSIGNED)
		                 | (rhs_flags & QUAL_UNSIGNED);
	} else if (lhs_type == TYPE_INT && rhs_type == TYPE_INT) {
		return TYPE_INT | (lhs_flags & QUAL_UNSIGNED)
		                | (rhs_flags & QUAL_UNSIGNED);
	} else if (lhs_type == TYPE_INT) {
		return lhs_flags;
	} else {
		return rhs_flags;
	}
}

/*
 * check_equality_type:
 * Check the types of the operands to an (in)equality operator.
 */
static void check_equality_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;

	lhs_flags = expr->left->expr_flags;
	rhs_flags = expr->right->expr_flags;

	if (FLAGS_IS_PTR(lhs_flags) && FLAGS_IS_PTR(rhs_flags)) {
		if (lhs_flags != rhs_flags)
			warning_imcompatible_ptr_cmp(expr);
		expr->expr_flags = TYPE_INT;
		return;
	} else if (FLAGS_IS_PTR(lhs_flags)) {
		if (!FLAGS_IS_INTEGER(rhs_flags))
			goto err_incompatible;

		warning_ptr_int_cmp(expr);
		expr->expr_flags = TYPE_INT;
		return;
	} else if (FLAGS_IS_PTR(rhs_flags)) {
		if (!FLAGS_IS_INTEGER(lhs_flags))
			goto err_incompatible;

		warning_ptr_int_cmp(expr);
		expr->expr_flags = TYPE_INT;
		return;
	} else if (FLAGS_IS_INTEGER(lhs_flags) && FLAGS_IS_INTEGER(rhs_flags)) {
		expr->expr_flags = TYPE_INT;
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * check_bitop_type:
 * Confirm that LHS and RHS of a bitwise operation are compatible.
 */
static void check_bitop_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;

	lhs_flags = expr->left->expr_flags;

	if (expr->tag == EXPR_NOT) {
		if (FLAGS_IS_PTR(lhs_flags) || !FLAGS_IS_INTEGER(lhs_flags))
			goto err_incompatible;

		expr->expr_flags = lhs_flags;
		return;
	}

	rhs_flags = expr->right->expr_flags;

	if (FLAGS_IS_PTR(lhs_flags) || FLAGS_IS_PTR(rhs_flags))
		goto err_incompatible;

	if (FLAGS_IS_INTEGER(lhs_flags) && FLAGS_IS_INTEGER(rhs_flags)) {
		/* Bitwise operations can only be performed on integer types. */
		expr->expr_flags = integer_type_convert(lhs_flags, rhs_flags);
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * check_additive_type:
 * Confirm that the LHS and RHS of an additive expression
 * are compatible and set the type of the expression.
 */
static void check_additive_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;

	lhs_flags = expr->left->expr_flags;
	rhs_flags = expr->right->expr_flags;

	if (FLAGS_IS_PTR(lhs_flags) && FLAGS_IS_PTR(rhs_flags)) {
		/*
		 * Two pointers can be subtracted only
		 * if they are of the same type.
		 * The resulting expression type is int.
		 * (Well, actually it's ptrdiff_t (signed long),
		 * but we don't support that.)
		 */
		if (expr->tag == EXPR_SUB && lhs_flags == rhs_flags) {
			expr->expr_flags = TYPE_INT;
			return;
		} else {
			goto err_incompatible;
		}
	}
	if (FLAGS_IS_PTR(lhs_flags)) {
		/* An integer can be added to or subtracted from a pointer. */
		if (FLAGS_IS_INTEGER(rhs_flags) &&
		    FLAGS_TYPE(lhs_flags) != TYPE_VOID) {
			expr->expr_flags = lhs_flags;
			pointer_additive_scale(expr);
			return;
		} else {
			goto err_incompatible;
		}
	}
	if (FLAGS_IS_PTR(rhs_flags)) {
		/* A pointer can be added to an integer, but not subtracted. */
		if (expr->tag == EXPR_ADD && FLAGS_IS_INTEGER(lhs_flags) &&
		    FLAGS_TYPE(rhs_flags) != TYPE_VOID) {
			expr->expr_flags = rhs_flags;
			pointer_additive_scale(expr);
			return;
		} else {
			goto err_incompatible;
		}
	}
	if (FLAGS_IS_INTEGER(lhs_flags) && FLAGS_IS_INTEGER(rhs_flags)) {
		/* Two integer types can be added or subtracted. */
		expr->expr_flags = integer_type_convert(lhs_flags, rhs_flags);
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * check_multiplicative_type:
 * Confirm that the LHS and RHS of a multiplicative expression
 * are compatible and set resulting expression type.
 */
static void check_multiplicative_type(struct ast_node *expr)
{
	unsigned int lhs_flags, rhs_flags;

	lhs_flags = expr->left->expr_flags;
	rhs_flags = expr->right->expr_flags;

	if (FLAGS_IS_PTR(lhs_flags) || FLAGS_IS_PTR(rhs_flags))
		goto err_incompatible;

	if (FLAGS_IS_INTEGER(lhs_flags) && FLAGS_IS_INTEGER(rhs_flags)) {
		/*
		 * Multiplicative operations can only
		 * be performed on integer types.
		 */
		expr->expr_flags = integer_type_convert(lhs_flags, rhs_flags);
		return;
	}

err_incompatible:
	error_incompatible_op_types(expr);
	exit(1);
}

/*
 * check_address_type:
 * Set the type of an address-of expression
 * by increasing the level of indirection.
 */
static void check_address_type(struct ast_node *expr)
{
	unsigned int indirection;

	if (!is_lvalue(expr->left)) {
		error_address_type(expr);
		exit(1);
	}

	expr->expr_flags = expr->left->expr_flags;
	indirection = (expr->expr_flags >> 24) + 1;
	expr->expr_flags &= 0x00FFFFFF;
	expr->expr_flags |= indirection << 24;
}

/*
 * check_dereference_type:
 * Check whether the operand of a dereference expression can be dereferenced
 * and decrease the exprssion's level of indirection.
 */
static void check_dereference_type(struct ast_node *expr)
{
	unsigned int indirection, flags;

	flags = expr->left->expr_flags;

	/*
	 * If the operand is not a pointer type, or if it is a singly
	 * indirect pointer to void (i.e. `void *`), it cannot be dereferenced.
	 */
	if (!FLAGS_IS_PTR(flags) || (FLAGS_TYPE(flags) == TYPE_VOID
	                             && FLAGS_INDIRECTION(flags) == 1)) {
		error_incompatible_op_types(expr);
		exit(1);
	}

	expr->expr_flags = expr->left->expr_flags;
	indirection = (expr->expr_flags >> 24) - 1;
	expr->expr_flags &= 0x00FFFFFF;
	expr->expr_flags |= indirection << 24;
}

/*
 * check_unary_type:
 * Check the type of the operand for a unary arithmetic operator.
 */
static void check_unary_type(struct ast_node *expr)
{
	unsigned int lhs_type = expr->left->expr_flags;

	if (!FLAGS_IS_INTEGER(lhs_type) || FLAGS_IS_PTR(lhs_type)) {
		error_incompatible_op_types(expr);
		exit(1);
	}
	expr->expr_flags = lhs_type;
}

static void check_func_type(struct ast_node *expr)
{
	expr->expr_flags = expr->left->expr_flags;
}

static void (*expr_type_func[])(struct ast_node *) = {
	[EXPR_ASSIGN]           = check_assign_type,
	[EXPR_LOGICAL_OR]       = check_boolean_type,
	[EXPR_LOGICAL_AND]      = check_boolean_type,
	[EXPR_OR]               = check_bitop_type,
	[EXPR_XOR]              = check_bitop_type,
	[EXPR_AND]              = check_bitop_type,
	[EXPR_EQ]               = check_equality_type,
	[EXPR_NE]               = check_equality_type,
	[EXPR_LT]               = check_equality_type,
	[EXPR_GT]               = check_equality_type,
	[EXPR_LE]               = check_equality_type,
	[EXPR_GE]               = check_equality_type,
	[EXPR_LSHIFT]           = check_bitop_type,
	[EXPR_RSHIFT]           = check_bitop_type,
	[EXPR_ADD]              = check_additive_type,
	[EXPR_SUB]              = check_additive_type,
	[EXPR_MULT]             = check_multiplicative_type,
	[EXPR_DIV]              = check_multiplicative_type,
	[EXPR_MOD]              = check_multiplicative_type,
	[EXPR_ADDRESS]          = check_address_type,
	[EXPR_DEREFERENCE]      = check_dereference_type,
	[EXPR_UNARY_PLUS]       = check_unary_type,
	[EXPR_UNARY_MINUS]      = check_unary_type,
	[EXPR_NOT]              = check_bitop_type,
	[EXPR_LOGICAL_NOT]      = check_boolean_type,
	[EXPR_FUNC]             = check_func_type
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
	default:
		expr_type_func[expr->tag](expr);
		break;
	}
}

static void print_type(FILE *f, struct ast_node *expr)
{
	int i;

	putc('[', f);
	if (expr->expr_flags & QUAL_UNSIGNED)
		fprintf(f, "unsigned ");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_INT)
		fprintf(f, "int");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_CHAR)
		fprintf(f, "char");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_VOID)
		fprintf(f, "void");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_STRLIT)
		fprintf(f, "const char[%lu]", strlen(expr->lexeme) - 1);

	i = FLAGS_INDIRECTION(expr->expr_flags);
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
	print_type(f, root);

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
