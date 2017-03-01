/*
 * symtab.h
 */

#ifndef FCC_SYMTAB_H
#define FCC_SYMTAB_H

#include "uthash.h"

/*
 * An entry in the symbol table.
 */
struct symbol {
	char            *id;
	unsigned int    flags;
	void            *extra;
	UT_hash_handle  hh;
};

struct symbol *symtab_entry(char *id);
struct symbol *symtab_entry_scope(char *id);
struct symbol *symtab_add(char *id, unsigned int flags);
struct symbol *symtab_add_func(char *id, unsigned int flags, void *params);

void symtab_init(void);
void symtab_new_scope(void);
void symtab_destroy_scope(void);

#endif /* FCC_SYMTAB_H */
