/*
 * symtab.h
 * Symbol table management functions.
 */

#include <stdlib.h>
#include <string.h>

#include "symtab.h"

static struct symbol *symtab = NULL;

/*
 * symtab_entry:
 * Fetch the entry for `id` from the symbol table,
 * or create one if it doesn't exist.
 */
struct symbol *symtab_entry(char *id)
{
	struct symbol *s;

	HASH_FIND_STR(symtab, id, s);
	if (!s) {
		s = malloc(sizeof *s);
		s->id = strdup(id);
		s->flags = 0;
		HASH_ADD_KEYPTR(hh, symtab, s->id, strlen(s->id), s);
	}

	return s;
}
