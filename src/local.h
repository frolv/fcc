/*
 * local.h
 */

#ifndef FCC_LOCAL_H
#define FCC_LOCAL_H

#include <stdlib.h>

#include "vector.h"
#include "types.h"

#define LFLAGS_USED 0x1

#define LFLAGS_REG_SHIFT 8
#define LFLAGS_REG(lflags) (((lflags) >> LFLAGS_REG_SHIFT) & 0xF)
#define LFLAGS_SET_REG(lflags, reg) \
	(((lflags) & ~0x00000F00) | ((reg) << LFLAGS_REG_SHIFT))

/* A local variable within a function. */
struct local {
	const char              *name;  /* variable name */
	off_t                   offset; /* offset from base pointer */
	struct type_information type;   /* type flags; see types.h */
	unsigned int            flags;  /* various flags */
};

struct local_vars {
	struct vector locals;
};

void local_init(struct local_vars *locals);
void local_destroy(struct local_vars *locals);

void local_add(struct local_vars *locals, const char *name,
               struct type_information *type);
void local_mark_used(struct local_vars *locals, const char *name);
struct local *local_find(struct local_vars *locals, const char *name);

#endif /* FCC_LOCAL_H */
