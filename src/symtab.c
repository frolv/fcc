/*
 * symtab.h
 * Symbol table management functions.
 */

#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "symtab.h"

static struct symbol **symtab_stack;
static size_t allocated_space;
static size_t ntables;

/*
 * symtab_entry:
 * Lookup `id` in the symbol table stack, starting with the most recent table.
 */
struct symbol *symtab_entry(char *id)
{
	int i;
	struct symbol *s = NULL;

	/* Search backwards through symbol table stack for id. */
	for (i = ntables - 1; i >= 0 && !s; --i)
		HASH_FIND_STR(symtab_stack[i], id, s);

	return s;
}

/*
 * symtab_entry_scope:
 * Lookup `id` in the current symbol table scope.
 */
struct symbol *symtab_entry_scope(char *id)
{
	struct symbol *s;

	HASH_FIND_STR(symtab_stack[ntables - 1], id, s);
	return s;
}

/*
 * symtab_add:
 * Add a new symbol to the current symbol table
 * with the specified ID and type flags.
 */
struct symbol *symtab_add(char *id, unsigned int flags)
{
	struct symbol *s;

	HASH_FIND_STR(symtab_stack[ntables - 1], id, s);
	if (!s) {
		s = malloc(sizeof *s);
		s->id = strdup(id);
		s->flags = flags;
		s->extra = NULL;

		HASH_ADD_KEYPTR(hh, symtab_stack[ntables - 1],
		                s->id, strlen(s->id), s);
	}
	return s;
}

static void create_param_array(struct symbol *s, struct ast_node *params);

/*
 * symtab_add_func:
 * Add a symbol for a function to the symbol table.
 * Params is the AST specifiying the function's parameter declarations.
 */
struct symbol *symtab_add_func(char *id, unsigned int flags, void *params)
{
	struct symbol *s;

	HASH_FIND_STR(symtab_stack[0], id, s);
	if (!s) {
		s = malloc(sizeof *s);
		s->id = strdup(id);
		s->flags = flags | PROPERTY_FUNC;
		create_param_array(s, params);

		HASH_ADD_KEYPTR(hh, symtab_stack[0], s->id, strlen(s->id), s);
	}
	return s;
}

void symtab_init(void)
{
	allocated_space = 8;
	symtab_stack = calloc(allocated_space, sizeof *symtab_stack);
	ntables = 1;
}

/* symtab_new_scope: create a new symbol table for a new scope */
void symtab_new_scope(void)
{
	if (ntables == allocated_space) {
		allocated_space *= 2;
		symtab_stack = realloc(symtab_stack, allocated_space
		                       * sizeof *symtab_stack);

		memset(symtab_stack + ntables, 0,
		       (allocated_space - ntables) * sizeof *symtab_stack);
	}
	++ntables;
}

/*
 * symtab_destroy_scope:
 * Destroy all entries in the current symbol table and return to previous scope.
 */
void symtab_destroy_scope(void)
{
	struct symbol *s, *tmp;

	--ntables;
	HASH_ITER(hh, symtab_stack[ntables], s, tmp) {
		HASH_DEL(symtab_stack[ntables], s);
		free(s->id);
		free(s);
	}
}

static void create_param_array(struct symbol *s, struct ast_node *params)
{
	s->extra = NULL;
	(void)params;
}
