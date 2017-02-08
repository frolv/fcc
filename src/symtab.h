/*
 * symtab.h
 */

#ifndef FCC_SYMTAB_H
#define FCC_SYMTAB_H

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
#define FLAGS_IS_PTR(x) ((x) & 0xFF000000)
#define FLAGS_IS_FUNC(x) ((x) & PROPERTY_FUNC)
#define FLAGS_IS_INTEGER(x) \
	(FLAGS_TYPE(x) == TYPE_INT || FLAGS_TYPE(x) == TYPE_CHAR)
#define FLAGS_INDIRECTION(x) ((x) >> 24)

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

#endif /* FCC_SYMTAB_H */
