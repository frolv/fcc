/*
 * error.h
 */

#ifndef FCC_ERROR_H
#define FCC_ERROR_H

#include "asg.h"
#include "ast.h"

void error_incompatible_op_types(struct ast_node *expr);
void error_incompatible_uplus(struct ast_node *operand);
void error_assign_type(struct ast_node *expr);
void error_address_type(struct ast_node *expr);
void error_undeclared(const char *id);
void error_declared(const char *id);
void error_struct_redefinition(const char *name);
void error_struct_undefined(const char *name);
void error_not_struct(struct ast_node *expr);
void error_struct_pointer(struct ast_node *expr);
void error_struct_member(struct ast_node *expr);

void warning_imcompatible_ptr_assn(struct ast_node *expr);
void warning_imcompatible_ptr_cmp(struct ast_node *expr);
void warning_ptr_int_cmp(struct ast_node *expr);
void warning_int_assign(struct ast_node *expr);
void warning_ptr_assign(struct ast_node *expr);
void warning_unreachable(struct graph_node *statement);
void warning_unused(const char *fname, const char *vname);

#endif /* FCC_ERROR_H */
