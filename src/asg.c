/*
 * asg.c
 * Functions to manage creation and manipulation of ASG nodes.
 */

#include <stdlib.h>

#include "asg.h"

struct graph_node *create_statement(struct ast_node *ast)
{
	struct asg_node_statement *s;

	s = malloc(sizeof *s);
	s->type = ASG_NODE_STATEMENT;
	s->next = NULL;
	s->ast = ast;

	return (struct graph_node *)s;
}

struct graph_node *create_conditional(struct ast_node *cond,
                                      struct graph_node *success,
                                      struct graph_node *failure)
{
	struct asg_node_conditional *c;

	c = malloc(sizeof *c);
	c->type = ASG_NODE_CONDITIONAL;
	c->next = NULL;
	c->cond = cond;
	c->succ = success;
	c->fail = failure;

	return (struct graph_node *)c;
}

struct graph_node *asg_append(struct graph_node *head, struct graph_node *n)
{
	struct graph_node *curr;

	if (!head)
		return n;

	for (curr = head; curr->next; curr = curr->next)
		;

	curr->next = n;
	return head;
}

static void print_asg_statement(struct graph_node *g)
{
	print_ast(stdout, ((struct asg_node_statement *)g)->ast);
}

static void print_asg_conditional(struct graph_node *g)
{
	struct asg_node_conditional *c = (struct asg_node_conditional *)g;

	printf("\x1B[1;32m===============IF===============\x1B[0;37m\n");
	print_ast(stdout, c->cond);
	printf("\x1B[1;32m==============THEN==============\x1B[0;37m\n");
	print_asg(c->succ);
	if (c->fail) {
		printf("\x1B[1;32m==============ELSE==============\x1B[0;37m\n");
		print_asg(c->fail);
	}
	printf("\x1B[1;32m=============END IF=============\x1B[0;37m\n\n");
}

static void (*print_asg_node[])(struct graph_node *) = {
	[ASG_NODE_STATEMENT] = print_asg_statement,
	[ASG_NODE_CONDITIONAL] = print_asg_conditional
};

void print_asg(struct graph_node *graph)
{
	for (; graph; graph = graph->next)
		print_asg_node[graph->type](graph);
}
