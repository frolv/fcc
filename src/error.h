/*
 * error.h
 */

#ifndef FCC_ERROR_H
#define FCC_ERROR_H

#include "ast.h"

void error_incompatible_op_types(struct ast_node *expr);
void error_assign_type(struct ast_node *expr);
void error_undeclared(char *id);
void error_declared(char *id);

void warning_imcompatible_ptr_assn(struct ast_node *expr);
void warning_imcompatible_ptr_cmp(struct ast_node *expr);
void warning_ptr_int_cmp(struct ast_node *expr);
void warning_int_assign(struct ast_node *expr);

#endif /* FCC_ERROR_H */
