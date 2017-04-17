/*
 * src/asg.h
 * Copyright (C) 2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FCC_ASG_H
#define FCC_ASG_H

#include "ast.h"

enum {
	ASG_NODE_DECLARATION,
	ASG_NODE_STATEMENT,
	ASG_NODE_CONDITIONAL,
	ASG_NODE_FOR,
	ASG_NODE_WHILE,
	ASG_NODE_DO_WHILE,
	ASG_NODE_RETURN
};

struct graph_node {
	int type;
	struct graph_node *next;
};

struct asg_node_statement {
	int type;
	struct graph_node *next;
	struct ast_node *ast;
};

struct asg_node_conditional {
	int type;
	struct graph_node *next;
	struct ast_node *cond;
	struct graph_node *succ;
	struct graph_node *fail;
};

struct asg_node_for {
	int type;
	struct graph_node *next;
	struct ast_node *init;
	struct ast_node *cond;
	struct ast_node *post;
	struct graph_node *body;
};

struct asg_node_while {
	int type;
	struct graph_node *next;
	struct ast_node *cond;
	struct graph_node *body;
};

struct asg_node_return {
	int type;
	struct graph_node *next;
	struct ast_node *retval;
};

struct graph_node *create_declaration(struct ast_node *ast);
struct graph_node *create_statement(struct ast_node *ast);
struct graph_node *create_conditional(struct ast_node *cond,
                                      struct graph_node *success,
                                      struct graph_node *failure);
struct graph_node *create_for_loop(struct ast_node *init,
                                   struct ast_node *cond,
                                   struct ast_node *post,
                                   struct graph_node *body);
struct graph_node *create_while_loop(int while_type,
                                     struct ast_node *cond,
                                     struct graph_node *body);
struct graph_node *create_return(struct ast_node *retval);

struct graph_node *asg_append(struct graph_node *head, struct graph_node *n);

void print_asg(struct graph_node *graph);

#endif /* FCC_ASG_H */
