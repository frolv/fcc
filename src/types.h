/*
 * src/types.h
 * Copyright (C) 2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FCC_TYPES_H
#define FCC_TYPES_H

#include <stddef.h>

#include "ast.h"
#include "vector.h"
#include "uthash.h"

enum {
	TYPE_INT = 1,
	TYPE_CHAR,
	TYPE_VOID,
	TYPE_STRLIT,
	TYPE_STRUCT
};

struct struct_struct {
	const char *name;
	size_t size;
	struct vector members;
	UT_hash_handle hh;
};

struct struct_member {
	const char *name;
	struct type_information type;
	size_t offset;
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

#define FLAGS_INDIRECTION_SHIFT 24

#define FLAGS_TYPE(x) ((x) & 0xF)
#define FLAGS_IS_PTR(x) ((x) & 0xFF000000)
#define FLAGS_IS_FUNC(x) ((x) & PROPERTY_FUNC)
#define FLAGS_IS_INTEGER(x) \
	(FLAGS_TYPE(x) == TYPE_INT || FLAGS_TYPE(x) == TYPE_CHAR)
#define FLAGS_INDIRECTION(x) ((x) >> FLAGS_INDIRECTION_SHIFT)

size_t type_size(struct type_information *type);

struct struct_struct *struct_create(const char *name, struct ast_node *members);
struct struct_struct *struct_find(const char *name);
struct struct_member *struct_get_member(struct struct_struct *s,
                                        const char *name);

#endif /* FCC_TYPES_H */
