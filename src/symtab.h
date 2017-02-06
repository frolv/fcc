/*
 * symtab.h
 */

#ifndef SYMTAB_H
#define SYMTAB_H

enum {
	TYPE_INT = 1,
	TYPE_CHAR,
	TYPE_VOID,
	TYPE_STRLIT
};

#define PROPERTY_FUNC   (1 << 8)
#define QUAL_UNSIGNED   (1 << 16)

#include "uthash.h"

#define FLAGS_TYPE(x) ((x) & 0xF)

/*
 * An entry in the symbol table.
 * The flags bitfield is set up as follows:
 * PPPPPPPPxxxxxxxUxxxxxxxFxxxxTTTT
 * P: level of indirection
 * U: unsigned flag
 * F: is a function
 * T: type
 */
struct symbol {
	char            *id;
	unsigned int    flags;
	UT_hash_handle  hh;
};

struct symbol *symtab_entry(char *id);

#endif /* SYMTAB_H */
