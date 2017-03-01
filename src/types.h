/*
 * types.h
 * Variable type information and functions.
 */

#include <stddef.h>

enum {
	TYPE_INT = 1,
	TYPE_CHAR,
	TYPE_VOID,
	TYPE_STRLIT
};

/*
 * A type flags bitfield is set up as follows:
 * PPPPPPPPxxxxxxxUxxxxxxxFxxxxTTTT
 * P: level of indirection
 * U: unsigned flag
 * F: is a function
 * T: type
 */

#define PROPERTY_FUNC   (1 << 8)
#define QUAL_UNSIGNED   (1 << 16)

#define FLAGS_TYPE(x) ((x) & 0xF)
#define FLAGS_IS_PTR(x) ((x) & 0xFF000000)
#define FLAGS_IS_FUNC(x) ((x) & PROPERTY_FUNC)
#define FLAGS_IS_INTEGER(x) \
	(FLAGS_TYPE(x) == TYPE_INT || FLAGS_TYPE(x) == TYPE_CHAR)
#define FLAGS_INDIRECTION(x) ((x) >> 24)

size_t type_size(unsigned int type_flags);
