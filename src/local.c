/*
 * local.c
 * Local variable tracking.
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
