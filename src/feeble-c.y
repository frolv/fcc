/*
 * Feeble C yacc/bison file.
 * Grammar adapted from http://www.quut.com/c/ANSI-C-grammar-y-2011.html
 */

%{
#include <stdio.h>

#include "asg.h"
#include "ast.h"
#include "error.h"
#include "fcc.h"
#include "gen.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"
#include "types.h"

void yyerror(yyscan_t scanner, char *err)
{
	fprintf(stderr, "\x1B[1;37m%s: line %d: \x1B[1;31merror:\x1B[0;37m %s\n",
	        fcc_filename, yyget_lineno(scanner), err);
}
%}

%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void * yyscan_t;
#endif

struct type_information {
	unsigned int type_flags;
	void *extra;
};
}

%union {
	unsigned int value;
	struct type_information type;
	struct ast_node *node;
	struct graph_node *graph;
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

%start translation_unit

%type <type> type_specifier
%type <type> type_specifiers
%type <type> type_name
%type <type> declaration_specifiers
%type <type> pointer
%type <type> struct_or_union
%type <type> struct_or_union_specifier
%type <type> struct_id
%type <type> struct_declaration_list

%type <value> unary_op

%type <node> direct_declarator
%type <node> declarator_list
%type <node> declarator
%type <node> parameter_list
%type <node> parameter_declaration
%type <node> expr
%type <node> assign_expr
%type <node> logical_or_expr
%type <node> logical_and_expr
%type <node> or_expr
%type <node> xor_expr
%type <node> and_expr
%type <node> equality_expr
%type <node> relational_expr
%type <node> shift_expr
%type <node> additive_expr
%type <node> multiplicative_expr
%type <node> cast_expr
%type <node> unary_expr
%type <node> postfix_expr
%type <node> expression
%type <node> argument_list

%type <graph> function_def
%type <graph> statement_block
%type <graph> statement_block_noscope
%type <graph> block_item
%type <graph> block_item_list
%type <graph> declaration
%type <graph> statement
%type <graph> expression_statement
%type <graph> conditional_statement
%type <graph> iteration_statement
%type <graph> jump_statement

%%

translation_unit
	: extern_decl
	| translation_unit extern_decl
	;

/*
 * Anything that can appear as a top-level construct in feeble C.
 */
extern_decl
	: function_def
	;

function_def
	: type_specifiers { symtab_new_scope(); } declarator
	{ symtab_add_func($3->lexeme, &$1, $3->left); }
	statement_block_noscope {
		translate_function($3->lexeme, $3->left, $5);
		/* print_asg($5); */
		/* free_asg($5); */
		symtab_destroy_scope();
	}
	;

type_specifiers
	: type_specifier
	| type_specifiers type_specifier {
		if (FLAGS_TYPE($1.type_flags) && FLAGS_TYPE($2.type_flags)) {
			/* TODO: proper error handling */
			yyerror(scanner, "multiple types given for expression\n");
			exit(1);
		}
		$$.type_flags = $1.type_flags | $2.type_flags;
	}
	;

type_specifier
	: TOKEN_VOID { $$.type_flags = TYPE_VOID; }
	| TOKEN_CHAR { $$.type_flags = TYPE_CHAR; }
	| TOKEN_INT { $$.type_flags = TYPE_INT; }
	| TOKEN_SIGNED { $$.type_flags = 0; }
	| TOKEN_UNSIGNED { $$.type_flags = QUAL_UNSIGNED; }
	| struct_or_union_specifier
	;

type_name
	: type_specifiers
	| type_specifiers pointer {
		$$.type_flags = $1.type_flags | ($2.type_flags << 24);
	}
	;

struct_or_union_specifier
	: struct_or_union struct_id '{' { symtab_new_scope(); }
	  struct_declaration_list { symtab_destroy_scope(); } '}' {
		$$.type_flags = $1.type_flags;
		if (!($$.extra = struct_create($2.extra, $5.extra))) {
			error_struct_redefinition($2.extra);
			exit(1);
		}
	}
	| struct_or_union struct_id {
		$$.type_flags = $1.type_flags;
		if (!($$.extra = struct_find($2.extra))) {
			error_struct_undefined($2.extra);
			exit(1);
		}
	}
	;

struct_or_union
	: TOKEN_STRUCT { $$.type_flags = TYPE_STRUCT; }
	;

struct_id
	: TOKEN_ID { $$.extra = strdup(yyget_text(scanner)); }
	;

struct_declaration_list
	: declaration struct_declaration_list {
		$$.extra = create_expr(EXPR_COMMA,
		                       ((struct asg_node_statement *)$1)->ast,
		                       $2.extra);
	}
	| declaration { $$.extra = ((struct asg_node_statement *)$1)->ast; }
	;

/* Variable or function declarator. */
declarator
	: pointer direct_declarator {
		$2->sym->flags.type_flags |= ($1.type_flags << 24); $$ = $2;
	}
	| direct_declarator
	;

direct_declarator
	: TOKEN_ID { $$ = create_node(NODE_NEWID, yyget_text(scanner)); }
	| direct_declarator '(' parameter_list ')' { $1->left = $3; }
	| direct_declarator '(' ')' { $1->left = NULL; }
	;

/* List of parameters in function prototype. */
parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration {
		$$ = create_expr(EXPR_COMMA, $1, $3);
	}
	;

parameter_declaration
	: type_specifiers declarator {
		if (ast_decl_set_type($2, &$1) != 0) {
			free_tree($2);
			exit(1);
		}
		$$ = $2;
	}
	| type_specifiers { $$ = NULL; }
	;

statement_block
	: '{' '}' { $$ = NULL; }
	| '{' { symtab_new_scope(); } block_item_list
	  { symtab_destroy_scope(); } '}' { $$ = $3; }
	;

statement_block_noscope
	: '{' '}' { $$ = NULL; }
	| '{' block_item_list '}' { $$ = $2; }
	;

block_item_list
	: block_item
	| block_item_list block_item {
		if ($2)
			$$ = asg_append($1, $2);
	}
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
	: ';' { $$ = NULL; }
	| expr ';' { $$ = create_statement($1); }
	;

conditional_statement
	: TOKEN_IF '(' expr ')' statement TOKEN_ELSE statement {
		$$ = create_conditional($3, $5, $7);
	}
	| TOKEN_IF '(' expr ')' statement {
		$$ = create_conditional($3, $5, NULL);
	}
	;

iteration_statement
	: TOKEN_WHILE '(' expr ')' statement {
		$$ = create_while_loop(ASG_NODE_WHILE, $3, $5);
	}
	| TOKEN_DO statement TOKEN_WHILE '(' expr ')' ';' {
		$$ = create_while_loop(ASG_NODE_DO_WHILE, $5, $2);
	}
	/* | TOKEN_FOR '(' declaration expression_statement expr ')' statement */
	| TOKEN_FOR '(' expression_statement expression_statement expr ')' statement {
		$$ = create_for_loop(((struct asg_node_statement *)$3)->ast,
		                     ((struct asg_node_statement *)$4)->ast,
		                     $5, $7);
	}
	;

jump_statement
/* 	: TOKEN_CONTINUE ';' */
/* 	| TOKEN_BREAK ';' */
	: TOKEN_RETURN ';' { $$ = create_return(NULL); }
	| TOKEN_RETURN expr ';' { $$ = create_return($2); }
/* 	; */

declaration
	: declaration_specifiers declarator_list ';' {
		if (ast_decl_set_type($2, &$1) != 0) {
			free_tree($2);
			exit(1);
		}
		$$ = create_declaration($2);
	}
	| declaration_specifiers { $$ = NULL; }
	;

declaration_specifiers
	: type_specifier
	| type_specifier declaration_specifiers {
		if (FLAGS_TYPE($1.type_flags) && FLAGS_TYPE($2.type_flags)) {
			/* TODO: proper error handling */
			yyerror(scanner, "multiple types given for expression\n");
			exit(1);
		}
		$$.type_flags = $1.type_flags | $2.type_flags;
	}
	;

declarator_list
	: declarator
	| declarator ',' declarator_list {
		$$ = create_expr(EXPR_COMMA, $1, $3);
	}
	;

/* how many levels of indirection are you on? */
pointer
	: '*' pointer { $$.type_flags = $2.type_flags + 1; }
	| '*' { $$.type_flags = 1; }
	;

/* the following expressions appear in order of operator precedence */
expr
	: assign_expr
	| expr ',' assign_expr { $$ = create_expr(EXPR_COMMA, $1, $3); }
	;

assign_expr
	: logical_or_expr
	| unary_expr assign_op assign_expr {
		$$ = create_expr(EXPR_ASSIGN, $1, $3);
	}
	;

logical_or_expr
	: logical_and_expr
	| logical_or_expr TOKEN_OR logical_and_expr {
		$$ = create_expr(EXPR_LOGICAL_OR, $1, $3);
	}
	;

logical_and_expr
	: or_expr
	| logical_and_expr TOKEN_AND or_expr {
		$$ = create_expr(EXPR_LOGICAL_AND, $1, $3);
	}
	;

or_expr
	: xor_expr
	| or_expr '|' xor_expr { $$ = create_expr(EXPR_OR, $1, $3); }
	;

xor_expr
	: and_expr
	| xor_expr '^' and_expr { $$ = create_expr(EXPR_XOR, $1, $3); }
	;

and_expr
	: equality_expr
	| and_expr '&' equality_expr { $$ = create_expr(EXPR_AND, $1, $3); }
	;

equality_expr
	: relational_expr
	| equality_expr TOKEN_EQ relational_expr {
		$$ = create_expr(EXPR_EQ, $1, $3);
	}
	| equality_expr TOKEN_NEQ relational_expr {
		$$ = create_expr(EXPR_NE, $1, $3);
	}
	;

relational_expr
	: shift_expr
	| relational_expr '<' shift_expr { $$ = create_expr(EXPR_LT, $1, $3); }
	| relational_expr '>' shift_expr { $$ = create_expr(EXPR_GT, $1, $3); }
	| relational_expr TOKEN_LE shift_expr {
		$$ = create_expr(EXPR_LE, $1, $3);
	}
	| relational_expr TOKEN_GE shift_expr {
		$$ = create_expr(EXPR_GE, $1, $3);
	}
	;

shift_expr
	: additive_expr
	| shift_expr TOKEN_LSHIFT additive_expr {
		$$ = create_expr(EXPR_LSHIFT, $1, $3);
	}
	| shift_expr TOKEN_RSHIFT additive_expr {
		$$ = create_expr(EXPR_RSHIFT, $1, $3);
	}
	;

additive_expr
	: multiplicative_expr
	| additive_expr '+' multiplicative_expr {
		$$ = create_expr(EXPR_ADD, $1, $3);
	}
	| additive_expr '-' multiplicative_expr {
		$$ = create_expr(EXPR_SUB, $1, $3);
	}
	;

multiplicative_expr
	: cast_expr
	| multiplicative_expr '*' cast_expr {
		$$ = create_expr(EXPR_MULT, $1, $3);
	}
	| multiplicative_expr '/' cast_expr {
		$$ = create_expr(EXPR_DIV, $1, $3);
	}
	| multiplicative_expr '%' cast_expr {
		$$ = create_expr(EXPR_MOD, $1, $3);
	}
	;

/* almost there... */
cast_expr
	: unary_expr
	| '(' type_name ')' cast_expr {
		if (ast_cast($4, &$2) != 0) {
			yyerror(scanner, "invalid cast");
			exit(1);
		}
		$$ = $4;
	}
	;

unary_expr
	: postfix_expr
	/* | TOKEN_INC unary_expr */
	/* | TOKEN_DEC unary_expr */
	| unary_op cast_expr { $$ = create_expr($1, $2, NULL); }
	/* | TOKEN_SIZEOF unary_expr */
	/* | TOKEN_SIZEOF '(' type_specifiers ')' */
	;

postfix_expr
	: expression
	| postfix_expr '[' expr ']' {
		$$ = create_expr(EXPR_DEREFERENCE,
		                 create_expr(EXPR_ADD, $1, $3),
		                 NULL);
	}
	| postfix_expr '.' TOKEN_ID {
		$$ = create_expr(EXPR_MEMBER, $1,
		                 create_node(NODE_MEMBER, yyget_text(scanner)));
	}
	| postfix_expr TOKEN_PTR TOKEN_ID {
		$$ = create_expr(EXPR_MEMBER,
		                 create_expr(EXPR_DEREFERENCE, $1, NULL),
		                 create_node(NODE_MEMBER, yyget_text(scanner)));
	}
	| postfix_expr TOKEN_INC
	| postfix_expr TOKEN_DEC
	| postfix_expr '(' ')' { $$ = create_expr(EXPR_FUNC, $1, NULL); }
	| postfix_expr '(' argument_list ')' {
		$$ = create_expr(EXPR_FUNC, $1, $3);
	}
	;

/* The lowest level expression with the highest precedence. */
expression
	: TOKEN_ID {
		$$ = create_node(NODE_IDENTIFIER, yyget_text(scanner));
	}
	| TOKEN_CONSTANT { $$ = create_node(NODE_CONSTANT, yyget_text(scanner)); }
	| TOKEN_STRLIT { $$ = create_node(NODE_STRLIT, yyget_text(scanner)); }
	| '(' expr ')' { $$ = $2; }
	;

unary_op
	: '&' { $$ = EXPR_ADDRESS; }
	| '*' { $$ = EXPR_DEREFERENCE; }
	| '+' { $$ = EXPR_UNARY_PLUS; }
	| '-' { $$ = EXPR_UNARY_MINUS; }
	| '~' { $$ = EXPR_NOT; }
	| '!' { $$ = EXPR_LOGICAL_NOT; }
	;

assign_op
	: '='
	;

argument_list
	: assign_expr
	| argument_list ',' assign_expr { $$ = create_expr(EXPR_COMMA, $1, $3); }
	;

%%
