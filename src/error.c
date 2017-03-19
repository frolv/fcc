/*
 * error.c
 * Error printing functions.
 */

#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "scan.h"

#include "fcc.h"
#include "error.h"
#include "types.h"

/*
 * This file contains ugly code. Turn around now.
 */

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

/*
 * You have been warned...
 */

static void err_print_type(struct ast_node *expr)
{
	unsigned int flags;
	int i;

	flags = expr->expr_flags.type_flags;
	if (flags & QUAL_UNSIGNED)
		fprintf(stderr, "unsigned ");

	if (FLAGS_TYPE(flags) == TYPE_INT)
		fprintf(stderr, "int");
	else if (FLAGS_TYPE(flags) == TYPE_CHAR)
		fprintf(stderr, "char");
	else if (FLAGS_TYPE(flags) == TYPE_VOID)
		fprintf(stderr, "void");
	else if (FLAGS_TYPE(flags) == TYPE_STRLIT)
		fprintf(stderr, "const char[%lu]", strlen(expr->lexeme) - 1);
	else if (FLAGS_TYPE(flags) == TYPE_STRUCT)
		fprintf(stderr, "struct %s",
		        ((struct struct_struct *)expr->expr_flags.extra)->name);


	i = FLAGS_INDIRECTION(flags);
	if (i) {
		fputc(' ', stderr);
		while (i--)
			fputc('*', stderr);
	}
}

#define PUTERR(fmt, ...) \
	fprintf(stderr, "\x1B[1;37m%s: line %u:\x1B[1;31m error:\x1B[0;37m " \
		fmt, fcc_filename, yyget_lineno(fcc_scanner), ##__VA_ARGS__)

#define PUTWARN(fmt, ...) \
	fprintf(stderr, "\x1B[1;37m%s: line %u:\x1B[1;35m warning:\x1B[0;37m " \
		fmt, fcc_filename, yyget_lineno(fcc_scanner), ##__VA_ARGS__)

void error_incompatible_op_types(struct ast_node *expr)
{
	int binop;

	binop = op_binary(expr->tag);

	PUTERR("incompatible type%s for %s %s operator: `\x1B[1;35m",
	       binop ? "s" : "", binop ? "binary" : "unary", op_sym[expr->tag]);

	err_print_type(expr->left);
	if (binop) {
		fprintf(stderr, "\x1B[0;37m' and `\x1B[1;35m");
		err_print_type(expr->right);
	}
	fprintf(stderr, "\x1B[0;37m'\n");
}

void error_incompatible_uplus(struct ast_node *operand)
{
	PUTERR("incompatible type for unary + operator: `\x1B[1;35m");
	err_print_type(operand);
	fprintf(stderr, "\x1B[0;37m'\n");
}

void error_assign_type(struct ast_node *expr)
{
	PUTERR("cannot assign to non-lvalue expression\n");
	/* may do something with this later */
	(void)expr;
}

void error_address_type(struct ast_node *expr)
{
	PUTERR("cannot take address of non-lvalue expression\n");
	/* may do something with this later */
	(void)expr;
}

void error_undeclared(char *id)
{
	PUTERR("undeclared identifier `%s'\n", id);
}

void error_declared(char *id)
{
	PUTERR("`%s' has already been declared in this scope\n", id);
}

void warning_imcompatible_ptr_assn(struct ast_node *expr)
{
	PUTWARN("assignment from incompatible pointer type: `\x1B[1;35m");
	err_print_type(expr->right);
	fprintf(stderr, "\x1B[0;37m' => `\x1B[1;35m");
	err_print_type(expr->left);
	fprintf(stderr, "\x1B[0;37m'\n");
}

void warning_imcompatible_ptr_cmp(struct ast_node *expr)
{
	PUTWARN("comparison between incompatible pointer types: `\x1B[1;35m");
	err_print_type(expr->left);
	fprintf(stderr, "\x1B[0;37m' and `\x1B[1;35m");
	err_print_type(expr->right);
	fprintf(stderr, "\x1B[0;37m'\n");
}

void warning_ptr_int_cmp(struct ast_node *expr)
{
	PUTWARN("comparison between integer and pointer without cast\n");
	(void)expr;
}

void warning_int_assign(struct ast_node *expr)
{
	PUTWARN("assigning integer to pointer without cast\n");
	(void)expr;
}

void warning_ptr_assign(struct ast_node *expr)
{
	PUTWARN("assigning pointer to integer without cast\n");
	(void)expr;
}

void warning_unreachable(struct graph_node *statement)
{
	PUTWARN("unreachable code\n");
	(void)statement;
}

void warning_unused(const char *fname, const char *vname)
{
	PUTWARN("unused variable `%s' in function `%s'\n", vname, fname);
}
