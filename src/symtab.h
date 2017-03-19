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
	char                    *id;
	struct type_information flags;
	void                    *extra;
	UT_hash_handle          hh;
};

struct symbol *symtab_entry(char *id);
struct symbol *symtab_entry_scope(char *id);
struct symbol *symtab_add(char *id, struct type_information *flags);
struct symbol *symtab_add_func(char *id, struct type_information *flags,
                               void *params);

void symtab_init(void);
void symtab_new_scope(void);
void symtab_destroy_scope(void);

#endif /* FCC_SYMTAB_H */
