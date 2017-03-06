/*
 * gen.c
 * Code generation functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asg.h"
#include "ast.h"
#include "error.h"
#include "fcc.h"
#include "gen.h"
#include "local.h"
#include "types.h"
#include "vector.h"
#include "x86.h"

#define SECTION_TEXT 0
#define SECTION_DATA 1

#define NUM_SECTIONS 2

struct section {
	size_t size;
	size_t len;
	char *buf;
};

static char *section_names[] = {
	[SECTION_TEXT] = "text",
	[SECTION_DATA] = "data"
};

static struct section sections[NUM_SECTIONS];

void begin_translation_unit(void)
{
	size_t i;

	for (i = 0; i < NUM_SECTIONS; ++i) {
		sections[i].size = 0x1000;
		sections[i].len = 0;
		sections[i].buf = malloc(sections[i].size);
		sections[i].buf[0] = '\0';
	}
}

void free_translation_unit(void)
{
	size_t i;

	for (i = 0; i < NUM_SECTIONS; ++i)
		free(sections[i].buf);
}

/* section_grow: double the buffer size of section `section` */
static inline void section_grow(int section)
{
	sections[section].size <<= 1;
	sections[section].buf = realloc(sections[section].buf,
	                                sections[section].size);
}

/*
 * section_write:
 * Write `len` bytes of `s` to section specified by `section`,
 * ensuring size.
 */
static void section_write(int section, const char *s, size_t len)
{
	if (sections[section].len + len >= sections[section].size)
		section_grow(section);

	strncpy(sections[section].buf + sections[section].len, s, len);
	sections[section].len += len;
}

static void check_usage(struct local_vars *locals, struct ast_node *ast)
{
	if (!ast) {
		return;
	} else if (ast->tag == NODE_IDENTIFIER) {
		local_mark_used(locals, ast->lexeme);
	} else {
		check_usage(locals, ast->left);
		check_usage(locals, ast->right);
	}
}

/*
 * add_locals:
 * Add all local variables in declaration statement `decl` to `l`.
 */
static void add_locals(struct local_vars *locals, struct ast_node *decl)
{
	/* tag is guaranteed to be IDENTIFIER or COMMA */
	if (decl->tag == NODE_IDENTIFIER) {
		local_add(locals, decl->lexeme, decl->expr_flags);
	} else {
		add_locals(locals, decl->left);
		add_locals(locals, decl->right);
	}
}

/*
 * scan_locals:
 * Find all local variables declared in `g`, and check if they get used.
 */
static void scan_locals(struct local_vars *locals, struct graph_node *g)
{
	if (!g)
		return;

	switch (g->type) {
	case ASG_NODE_DECLARATION:
		add_locals(locals, ((struct asg_node_statement *)g)->ast);
		break;
	case ASG_NODE_STATEMENT:
		check_usage(locals, ((struct asg_node_statement *)g)->ast);
		break;
	case ASG_NODE_CONDITIONAL:
		check_usage(locals, ((struct asg_node_conditional *)g)->cond);
		scan_locals(locals, ((struct asg_node_conditional *)g)->succ);
		scan_locals(locals, ((struct asg_node_conditional *)g)->fail);
		break;
	case ASG_NODE_FOR:
		check_usage(locals, ((struct asg_node_for *)g)->init);
		check_usage(locals, ((struct asg_node_for *)g)->cond);
		check_usage(locals, ((struct asg_node_for *)g)->post);
		scan_locals(locals, ((struct asg_node_for *)g)->body);
		break;
	case ASG_NODE_WHILE:
	case ASG_NODE_DO_WHILE:
		check_usage(locals, ((struct asg_node_while *)g)->cond);
		scan_locals(locals, ((struct asg_node_while *)g)->body);
		break;
	case ASG_NODE_RETURN:
		check_usage(locals, ((struct asg_node_return *)g)->retval);
		break;
	}
	scan_locals(locals, g->next);
}

static size_t read_locals(const char *fname,
                          struct local_vars *locals,
                          struct graph_node *g)
{
	size_t nbytes, size;
	struct local *l;

	scan_locals(locals, g);
	nbytes = 0;

	VECTOR_ITER(&locals->locals, l) {
		if (!(l->flags & LFLAGS_USED)) {
			warning_unused(fname, l->name);
			continue;
		}

		size = type_size(l->type);
		if (!ALIGNED(nbytes, size))
			nbytes = ALIGN(nbytes, size);

		nbytes += size;
		l->offset = nbytes;
	}
	if (!ALIGNED(nbytes, 4))
		nbytes = ALIGN(nbytes, 4);

	return nbytes;
}

static void write_x86(struct x86_sequence *x86)
{
	struct x86_instruction *x;
	char buf[64];
	size_t len;

	buf[0] = '\n';
	section_write(SECTION_TEXT, buf, 1);
	VECTOR_ITER(&x86->seq, x) {
		len = x86_write_instruction(x, buf);
		printf("%s", buf);
		section_write(SECTION_TEXT, buf, len);
	}
}

/*
 * translate_function:
 * Translate the ASG for a single C function to x86 assembly.
 */
void translate_function(const char *fname, struct graph_node *g)
{
	size_t bytes;
	struct local_vars locals;
	struct x86_sequence x86;

	local_init(&locals);
	x86_seq_init(&x86, &locals);

	bytes = read_locals(fname, &locals, g);

	x86_begin_function(&x86, fname);
	x86_grow_stack(&x86, bytes);
	x86_translate(&x86, g);
	x86_shrink_stack(&x86, bytes);
	x86_end_function(&x86);

	write_x86(&x86);

	x86_seq_destroy(&x86);
	local_destroy(&locals);
}

void flush_to_file(char *filename)
{
	FILE *f;
	int sec;

	f = fopen(filename, "w");
	for (sec = 0; sec < NUM_SECTIONS; ++sec) {
		if (!sections[sec].len)
			continue;

		fprintf(f, ".section .%s\n", section_names[sec]);
		fputs(sections[sec].buf, f);
	}
	fclose(f);
}
