/*
 * asg.c
 * Functions to manage creation and manipulation of ASG nodes.
 */

#include <stdlib.h>

#include "asg.h"

/*
 * create_statement:
 * Create a graph node representing a single, linearly executed statement.
 */
struct graph_node *create_statement(struct ast_node *ast)
{
	struct asg_node_statement *s;

	s = malloc(sizeof *s);
	s->type = ASG_NODE_STATEMENT;
	s->next = NULL;
	s->ast = ast;

	return (struct graph_node *)s;
}

/*
 * create_conditional:
 * Create a graph node representing a conditional statement with a test,
 * a graph of operations to perform on success, and optionally a graph of
 * operations to perform on failure.
 */
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

/*
 * create_for_loop:
 * Create a graph node representing a for loop consisting of three parts:
 * an initialization, a condition and an afterthought, with a body of
 * statements to run.
 */
struct graph_node *create_for_loop(struct ast_node *init,
                                   struct ast_node *cond,
                                   struct ast_node *post,
                                   struct graph_node *body)
{
	struct asg_node_for *f;

	f = malloc(sizeof *f);
	f->type = ASG_NODE_FOR;
	f->next = NULL;
	f->init = init;
	f->cond = cond;
	f->post = post;
	f->body = body;

	return (struct graph_node *)f;
}

struct graph_node *create_while_loop(int while_type,
                                     struct ast_node *cond,
                                     struct graph_node *body)
{
	struct asg_node_while *w;

	if (while_type != ASG_NODE_WHILE && while_type != ASG_NODE_DO_WHILE)
		return NULL;

	w = malloc(sizeof *w);
	w->type = while_type;
	w->next = NULL;
	w->cond = cond;
	w->body = body;

	return (struct graph_node *)w;
}

/*
 * asg_append:
 * Append node `n` to the end of the ASG starting at `head`.
 * 
 * TODO: this is pretty inefficient - perhaps store ASGs differently.
 */
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

/*
 * Functions to print out ASGs.
 */

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

static void print_asg_for_loop(struct graph_node *g)
{
	struct asg_node_for *f = (struct asg_node_for *)g;

	printf("\x1B[1;32m===============FOR==============\x1B[0;37m\n");
	print_ast(stdout, f->init);
	printf("\x1B[1;32m===============COND=============\x1B[0;37m\n");
	print_ast(stdout, f->cond);
	printf("\x1B[1;32m==============AFTER=============\x1B[0;37m\n");
	print_ast(stdout, f->post);
	printf("\x1B[1;32m===============BODY=============\x1B[0;37m\n");
	print_asg(f->body);
	printf("\x1B[1;32m==============ENDFOR============\x1B[0;37m\n\n");
}

static void print_asg_while_loop(struct graph_node *g)
{
	struct asg_node_while *w = (struct asg_node_while *)g;

	printf("\x1B[1;32m==============WHILE=============\x1B[0;37m\n");
	print_ast(stdout, w->cond);
	printf("\x1B[1;32m================DO==============\x1B[0;37m\n");
	print_asg(w->body);
	printf("\x1B[1;32m=============ENDWHILE===========\x1B[0;37m\n\n");
}

static void print_asg_do_while_loop(struct graph_node *g)
{
	struct asg_node_while *w = (struct asg_node_while *)g;

	printf("\x1B[1;32m================DO==============\x1B[0;37m\n");
	print_asg(w->body);
	printf("\x1B[1;32m==============WHILE=============\x1B[0;37m\n");
	print_ast(stdout, w->cond);
	printf("\x1B[1;32m============ENDDOWHILE==========\x1B[0;37m\n\n");
}

static void (*print_asg_node[])(struct graph_node *) = {
	[ASG_NODE_STATEMENT] = print_asg_statement,
	[ASG_NODE_CONDITIONAL] = print_asg_conditional,
	[ASG_NODE_FOR] = print_asg_for_loop,
	[ASG_NODE_WHILE] = print_asg_while_loop,
	[ASG_NODE_DO_WHILE] = print_asg_do_while_loop
};

void print_asg(struct graph_node *graph)
{
	for (; graph; graph = graph->next)
		print_asg_node[graph->type](graph);
}
