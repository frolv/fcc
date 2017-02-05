/*
 * Feeble C yacc/bison file.
 * Grammar adapted from http://www.quut.com/c/ANSI-C-grammar-y-2011.html
 */

%{
#include <stdio.h>

#include "ast.h"
#include "fcc.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"

void yyerror(yyscan_t scanner, char *err)
{
	fprintf(stderr, "%s: line %d: %s\n", fcc_filename,
	        yyget_lineno(scanner), err);
}
%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void * yyscan_t;
#endif
}

%union {
	unsigned int flags;
	struct ast_node *node;
}

/* Make the parser reentrant instead of using global variables. */
%define api.pure full
%lex-param   {yyscan_t scanner}
%parse-param {yyscan_t scanner}

%token TOKEN_ID TOKEN_CONSTANT TOKEN_STRLIT TOKEN_SIZEOF
%token TOKEN_INT TOKEN_CHAR TOKEN_VOID TOKEN_SIGNED TOKEN_UNSIGNED
%token TOKEN_IF TOKEN_ELSE TOKEN_FOR TOKEN_DO TOKEN_WHILE TOKEN_BREAK TOKEN_CONTINUE TOKEN_RETURN
%token TOKEN_STRUCT
%token TOKEN_AND TOKEN_OR TOKEN_EQ TOKEN_NEQ TOKEN_GE TOKEN_LE
%token TOKEN_LSHIFT TOKEN_RSHIFT TOKEN_INC TOKEN_DEC TOKEN_PTR

%start block_item_list

%type <flags> type_specifier
%type <flags> type_specifiers
%type <flags> declaration_specifiers
%type <flags> pointer

%type <node> direct_declarator
%type <node> declarator_list
%type <node> declarator

%%

translation_unit
	: extern_decl
	| translation_unit extern_decl
	;

/*
 * Anything that can appear as a top-level construct in feeble C.
 * At the moment, this is functions only, but global variables
 * can be added later.
 */
extern_decl
	: function_def
	;

function_def
	: type_specifiers declarator statement_block
	;

type_specifiers
	: type_specifier
	| type_specifiers type_specifier { $$ = $1 | $2; }
	;

type_specifier
	: TOKEN_VOID { $$ = TYPE_VOID; }
	| TOKEN_CHAR { $$ = TYPE_CHAR; }
	| TOKEN_INT { $$ = TYPE_INT; }
	| TOKEN_SIGNED { $$ = 0; }
	| TOKEN_UNSIGNED { $$ = QUAL_UNSIGNED; }
	;

/* Variable or function declarator. */
declarator
	: pointer direct_declarator { $2->sym->flags |= ($1 << 24); $$ = $2; }
	| direct_declarator
	;

direct_declarator
	: TOKEN_ID { $$ = create_node(NODE_IDENTIFIER, yyget_text(scanner)); }
	| direct_declarator '(' parameter_list ')'
	| direct_declarator '(' ')'
	;

/* List of parameters in function prototype. */
parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;

parameter_declaration
	: type_specifiers declarator
	| type_specifiers
	;

statement_block
	: '{' '}'
	| '{' block_item_list '}'
	;

block_item_list
	: block_item
	| block_item_list block_item
	;

block_item
	: declaration
	| statement
	;

statement
	: statement_block
	| expression_statement
	| conditional_statement
	| iteration_statement
	| jump_statement
	;

expression_statement
	: ';'
	| expr ';'
	;

conditional_statement
	: TOKEN_IF '(' expr ')' statement TOKEN_ELSE statement
	| TOKEN_IF '(' expr ')' statement
	;

iteration_statement
	: TOKEN_WHILE '(' expr ')' statement
	| TOKEN_DO statement TOKEN_WHILE '(' expr ')' ';'
	| TOKEN_FOR '(' declaration expression_statement expr ')' statement
	/* e.g. for (; cond; expr) ... */
	| TOKEN_FOR '(' expression_statement expression_statement expr ')' statement
	;

jump_statement
	: TOKEN_CONTINUE ';'
	| TOKEN_BREAK ';'
	| TOKEN_RETURN ';'
	| TOKEN_RETURN expr ';'
	;

declaration
	: declaration_specifiers declarator_list ';' {
		if (ast_decl_set_type($2, $1) != 0) {
			free_tree($2);
			exit(1);
		}
		print_ast(stdout, $2);
		free_tree($2);
	}
	;

declaration_specifiers
	: type_specifier
	| type_specifier declaration_specifiers { $$ = $1 | $2; }
	;

declarator_list
	: declarator
	| declarator ',' declarator_list {
		$$ = create_expr(EXPR_COMMA, 0, $1, $3);
	}
	;

/* how many levels of indirection are you on? */
pointer
	: '*' pointer { $$ = $2 + 1; }
	| '*' { $$ = 1; }
	;

/* the following expressions appear in order of operator precedence */
expr
	: assign_expr
	| expr ',' assign_expr
	;

assign_expr
	: logical_or_expr
	| unary_expr assign_op assign_expr
	;

logical_or_expr
	: logical_and_expr
	| logical_or_expr TOKEN_OR logical_and_expr
	;

logical_and_expr
	: or_expr
	| logical_and_expr TOKEN_AND or_expr
	;

or_expr
	: xor_expr
	| or_expr '|' xor_expr
	;

xor_expr
	: and_expr
	| xor_expr '^' and_expr
	;

and_expr
	: equality_expr
	| and_expr '&' equality_expr
	;

equality_expr
	: relational_expr
	| equality_expr TOKEN_EQ relational_expr
	| equality_expr TOKEN_NEQ relational_expr
	;

relational_expr
	: shift_expr
	| relational_expr '<' shift_expr
	| relational_expr '>' shift_expr
	| relational_expr TOKEN_LE shift_expr
	| relational_expr TOKEN_GE shift_expr
	;

shift_expr
	: additive_expr
	| shift_expr TOKEN_LSHIFT additive_expr
	| shift_expr TOKEN_RSHIFT additive_expr
	;

additive_expr
	: multiplicative_expr
	| additive_expr '+' multiplicative_expr
	| additive_expr '-' multiplicative_expr
	;

multiplicative_expr
	: cast_expr
	| multiplicative_expr '*' cast_expr
	| multiplicative_expr '/' cast_expr
	| multiplicative_expr '%' cast_expr
	;

/* almost there... */
cast_expr
	: unary_expr
	| '(' type_specifiers ')' cast_expr
	;

unary_expr
	: postfix_expr
	| TOKEN_INC unary_expr
	| TOKEN_DEC unary_expr
	| unary_op cast_expr
	| TOKEN_SIZEOF unary_expr
	| TOKEN_SIZEOF '(' type_specifiers ')'
	;

postfix_expr
	: expression
	| postfix_expr '[' expr ']'
	| postfix_expr '.' TOKEN_ID
	| postfix_expr TOKEN_PTR TOKEN_ID
	| postfix_expr TOKEN_INC
	| postfix_expr TOKEN_DEC
	| postfix_expr '(' ')'
	;

/* The lowest level expression with the highest precedence. */
expression
	: TOKEN_ID
	| TOKEN_CONSTANT
	| TOKEN_STRLIT
	| '(' expr ')'
	;

unary_op
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

assign_op
	: '='
	;

%%
