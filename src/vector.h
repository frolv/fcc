/*
 * src/vector.h
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

#ifndef FCC_VECTOR_H
#define FCC_VECTOR_H

#include <stddef.h>

struct vector {
	size_t allocated;
	size_t nmembs;
	size_t size;
	void   *data;
};

#define VECTOR_ITER(v, p) \
	for ((p) = (void *)(v)->data; \
	     ((char *)(p)) < (((char *)(v)->data) + (v)->nmembs * (v)->size); \
	     (p) = (void *)(((char *)(p)) + (v)->size))

void vector_init(struct vector *v, size_t elem_size);
void vector_destroy(struct vector *v);
void vector_clear(struct vector *v);

void vector_append(struct vector *v, void *elem);
int vector_pop(struct vector *v, void *ret);

int vector_set(struct vector *v, size_t index, void *elem);
int vector_get(struct vector *v, size_t index, void *ret);

#endif /* FCC_VECTOR_H */
