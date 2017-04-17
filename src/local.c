/*
 * src/local.c
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

#include <string.h>

#include "local.h"

void local_init(struct local_vars *locals)
{
	vector_init(&locals->locals, sizeof (struct local));
}

void local_destroy(struct local_vars *locals)
{
	vector_destroy(&locals->locals);
}

void local_add(struct local_vars *locals, const char *name,
               struct type_information *type)
{
	struct local l;

	memset(&l, 0, sizeof l);
	l.name = name;
	memcpy(&l.type, type, sizeof *type);
	vector_append(&locals->locals, &l);
}

void local_mark_used(struct local_vars *locals, const char *name)
{
	struct local *l;

	if ((l = local_find(locals, name)))
		l->flags |= LFLAGS_USED;
}

struct local *local_find(struct local_vars *locals, const char *name)
{
	struct local *l;

	VECTOR_ITER(&locals->locals, l) {
		if (strcmp(l->name, name) == 0)
			return l;
	}

	return NULL;
}
