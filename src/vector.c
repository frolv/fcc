/*
 * src/vector.c
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

#include <stdlib.h>
#include <string.h>

#include "vector.h"

/*
 * vector_init:
 * Initialize a vector to store elements of type `elem_size`.
 */
void vector_init(struct vector *v, size_t elem_size)
{
	v->allocated = 16;
	v->nmembs = 0;
	v->size = elem_size;
	v->data = malloc(v->allocated * v->size);
}

#define VECTOR_INDEX(v, i) \
	(((char *)(v)->data) + (i) * (v)->size)

/*
 * vector_append:
 * Append a copy of element `elem` to the end of vector.
 */
void vector_append(struct vector *v, void *elem)
{
	if (v->nmembs == v->allocated) {
		v->allocated <<= 1;
		v->data = realloc(v->data, v->allocated * v->size);
	}
	memcpy(VECTOR_INDEX(v, v->nmembs), elem, v->size);
	v->nmembs++;
}

/*
 * vector_clear:
 * Empty vector.
 */
void vector_clear(struct vector *v)
{
	v->nmembs = 0;
}

/*
 * vector_pop:
 * Remove the last element from vector and store it in `ret`.
 */
int vector_pop(struct vector *v, void *ret)
{
	if (!v->nmembs)
		return 1;

	v->nmembs--;
	if (ret)
		memcpy(ret, VECTOR_INDEX(v, v->nmembs), v->size);
	return 0;
}

/*
 * vector_set:
 * Set element at given index in vector to `elem`.
 */
int vector_set(struct vector *v, size_t index, void *elem)
{
	if (index >= v->nmembs)
		return 1;

	memcpy(VECTOR_INDEX(v, index), elem, v->size);
	return 0;
}

/*
 * vector_get:
 * Store element at given index of vector in `ret`.
 */
int vector_get(struct vector *v, size_t index, void *ret)
{
	if (index >= v->nmembs)
		return 1;

	memcpy(ret, VECTOR_INDEX(v, index), v->size);
	return 0;
}

void vector_destroy(struct vector *v)
{
	free(v->data);
}
