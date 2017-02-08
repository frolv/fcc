/*
 * error.c
 * Error printing functions.
 */

#include <stdio.h>
#include <string.h>

#include "error.h"
#include "symtab.h"

static inline int op_binary(int expr_tag)
{
	switch (expr_tag) {
	case EXPR_ASSIGN:
	case EXPR_LOGICAL_OR:
	case EXPR_LOGICAL_AND:
	case EXPR_OR:
	case EXPR_XOR:
	case EXPR_AND:
	case EXPR_EQ:
	case EXPR_NE:
	case EXPR_LT:
	case EXPR_GT:
	case EXPR_LE:
	case EXPR_GE:
	case EXPR_LSHIFT:
	case EXPR_RSHIFT:
	case EXPR_ADD:
	case EXPR_SUB:
	case EXPR_MULT:
	case EXPR_DIV:
	case EXPR_MOD:
		return 1;
	default:
		return 0;
	}
}

static char *op_sym[] = {
	[EXPR_ASSIGN]           = "=",
	[EXPR_LOGICAL_OR]       = "||",
	[EXPR_LOGICAL_AND]      = "&&",
	[EXPR_OR]               = "|",
	[EXPR_XOR]              = "^",
	[EXPR_AND]              = "&",
	[EXPR_EQ]               = "==",
	[EXPR_NE]               = "!=",
	[EXPR_LT]               = "<",
	[EXPR_GT]               = ">",
	[EXPR_LE]               = "<=",
	[EXPR_GE]               = ">=",
	[EXPR_LSHIFT]           = "<<",
	[EXPR_RSHIFT]           = ">>",
	[EXPR_ADD]              = "+",
	[EXPR_SUB]              = "-",
	[EXPR_MULT]             = "*",
	[EXPR_DIV]              = "/",
	[EXPR_MOD]              = "%",
	[EXPR_ADDRESS]          = "&",
	[EXPR_DEREFERENCE]      = "*",
	[EXPR_UNARY_PLUS]       = "+",
	[EXPR_UNARY_MINUS]      = "-",
	[EXPR_NOT]              = "~",
	[EXPR_LOGICAL_NOT]      = "!"
};

static void err_print_type(struct ast_node *expr)
{
	int i;

	if (expr->expr_flags & QUAL_UNSIGNED)
		fprintf(stderr, "unsigned ");

	if (FLAGS_TYPE(expr->expr_flags) == TYPE_INT)
		fprintf(stderr, "int");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_CHAR)
		fprintf(stderr, "char");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_VOID)
		fprintf(stderr, "void");
	if (FLAGS_TYPE(expr->expr_flags) == TYPE_STRLIT)
		fprintf(stderr, "const char[%lu]", strlen(expr->lexeme) - 1);

	i = FLAGS_INDIRECTION(expr->expr_flags);
	if (i) {
		fputc(' ', stderr);
		while (i--)
			fputc('*', stderr);
	}
}

void error_incompatible_op_types(struct ast_node *expr)
{
	int binop;

	binop = op_binary(expr->tag);

	fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
	        "incompatible type%s for %s %s operator: `\x1B[1;35m",
	        binop ? "s" : "", binop ? "binary" : "unary", op_sym[expr->tag]);

	err_print_type(expr->left);
	if (binop) {
		fprintf(stderr, "\x1B[0;37m' and `\x1B[1;35m");
		err_print_type(expr->right);
	}
	fprintf(stderr, "\x1B[0;37m'\n");
}

void error_assign_type(struct ast_node *expr)
{
	fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
	        "cannot assign to non-lvalue expression\n");
	/* may do something with this later */
	(void)expr;
}

void error_undeclared(struct ast_node *expr)
{
	fprintf(stderr, "\x1B[1;31merror:\x1B[0;37m "
	        "undeclared identifier: \x1B[1;35m%s\x1B[0;37m\n",
	        expr->lexeme);
}

void warning_imcompatible_ptr_assn(struct ast_node *expr)
{
	fprintf(stderr, "\x1B[1;35mwarning:\x1B[0;37m "
	        "assignment from incompatible pointer type: `\x1B[1;35m");
	err_print_type(expr->right);
	fprintf(stderr, "\x1B[0;37m' => `\x1B[1;35m");
	err_print_type(expr->left);
	fprintf(stderr, "\x1B[0;37m'\n");
}

void warning_imcompatible_ptr_cmp(struct ast_node *expr)
{
	fprintf(stderr, "\x1B[1;35mwarning:\x1B[0;37m "
	        "comparison between incompatible pointer types: `\x1B[1;35m");
	err_print_type(expr->left);
	fprintf(stderr, "\x1B[0;37m' and `\x1B[1;35m");
	err_print_type(expr->right);
	fprintf(stderr, "\x1B[0;37m'\n");
}

void warning_ptr_int_cmp(struct ast_node *expr)
{
	fprintf(stderr, "\x1B[1;35mwarning:\x1B[0;37m "
	        "comparison between integer and pointer without cast\n");
	(void)expr;
}