/*
 * symtab.h
 */

#ifndef SYMTAB_H
#define SYMTAB_H

#define TYPE_INT        (1 << 0)
#define TYPE_CHAR       (1 << 1)
#define TYPE_VOID       (1 << 2)

#define QUAL_UNSIGNED   (1 << 16)

#include "uthash.h"

/*
 * An entry in the symbol table.
 * The flags bitfield is set up as follows:
 * PPPPPPPPxxxxxxxUxxxxxVCI
 * P: level of indirection
 * U: unsigned flag
 * V: void type
 * C: char type
 * I: int type
 */
struct symbol {
	char            *id;
	unsigned int    flags;
	UT_hash_handle  hh;
};

struct symbol *symtab_entry(char *id);

#endif /* SYMTAB_H */
