/*
 * error.h
 */

#ifndef FCC_ERROR_H
#define FCC_ERROR_H

#include "ast.h"

void error_incompatible_op_types(struct ast_node *expr);

#endif /* FCC_ERROR_H */
