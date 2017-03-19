/*
 * types.c
 */

#include <stdlib.h>

#include "fcc.h"
#include "types.h"

static struct struct_struct *structs = NULL;

static size_t sizes[] = {
	[TYPE_INT] = 4,
	[TYPE_CHAR] = 1
};

size_t type_size(struct type_information *type)
{
	unsigned int t;

	t = FLAGS_TYPE(type->type_flags);
	if (t == TYPE_STRLIT || t == TYPE_VOID)
		return 0;
	else if (FLAGS_IS_PTR(type->type_flags))
	         return 4;
	else if (t == TYPE_STRUCT)
		return ((struct struct_struct *)type->extra)->size;
	else
		return sizes[t];
}

static void struct_add_members(struct struct_struct *s, struct ast_node *ast)
{
	size_t size;

	if (!ast) {
		return;
	} else if (ast->tag == NODE_IDENTIFIER) {
		size = type_size(&ast->expr_flags);
		if (!ALIGNED(size, 4))
			size = ALIGN(size, 4);
		s->size += size;
	} else if (ast->tag == EXPR_COMMA) {
		struct_add_members(s, ast->left);
		struct_add_members(s, ast->right);
	}
}

struct struct_struct *struct_create(const char *name, struct ast_node *members)
{
	struct struct_struct *s;

	s = malloc(sizeof *s);
	s->name = name;
	s->size = 0;
	struct_add_members(s, members);

	HASH_ADD_KEYPTR(hh, structs, s->name, strlen(s->name), s);

	return s;
}

struct struct_struct *struct_find(const char *name)
{
	struct struct_struct *s;

	HASH_FIND_STR(structs, name, s);
	return s;
}
