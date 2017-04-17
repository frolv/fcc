/*
 * src/types.c
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

#include <stdlib.h>

#include "fcc.h"
#include "types.h"

static size_t sizes[] = {
	[TYPE_INT] = 4,
	[TYPE_CHAR] = 1
};

size_t type_size(struct type_information *type)
{
	unsigned int t;

	t = FLAGS_TYPE(type->type_flags);
	if (t == TYPE_STRLIT)
		return 0;
	else if (FLAGS_IS_PTR(type->type_flags))
	         return 4;
	else if (t == TYPE_VOID)
		return 0;
	else if (t == TYPE_STRUCT)
		return ((struct struct_struct *)type->extra)->size;
	else
		return sizes[t];
}

static struct struct_struct *structs = NULL;

static void struct_add_members(struct struct_struct *s, struct ast_node *ast)
{
	struct struct_member member;
	size_t size;

	if (!ast) {
		return;
	} else if (ast->tag == NODE_IDENTIFIER) {
		size = type_size(&ast->expr_flags);
		if (!ALIGNED(s->size, size))
			s->size = ALIGN(s->size, size);

		member.name = ast->lexeme;
		memcpy(&member.type, &ast->expr_flags, sizeof member.type);
		member.offset = s->size;
		vector_append(&s->members, &member);

		s->size += size;
	} else if (ast->tag == EXPR_COMMA) {
		struct_add_members(s, ast->left);
		struct_add_members(s, ast->right);
	}
}

/*
 * struct_create:
 * Create a new struct called `name' with members
 * represented in AST of variable declarations.
 */
struct struct_struct *struct_create(const char *name, struct ast_node *members)
{
	struct struct_struct *s;

	HASH_FIND_STR(structs, name, s);
	if (s)
		return NULL;

	s = malloc(sizeof *s);
	s->name = name;
	s->size = 0;
	vector_init(&s->members, sizeof (struct struct_member));
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

struct struct_member *struct_get_member(struct struct_struct *s,
                                        const char *name)
{
	struct struct_member *m;

	VECTOR_ITER(&s->members, m) {
		if (strcmp(m->name, name) == 0)
			return m;
	}
	return NULL;
}
