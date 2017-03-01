/*
 * vector.c
 * Dynamic arrays.
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
	v->used = v->nmembs = 0;
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
	if (v->used == v->allocated) {
		v->allocated <<= 1;
		v->data = realloc(v->data, v->allocated * v->size);
	}
	memcpy(VECTOR_INDEX(v, v->nmembs), elem, v->size);
	v->nmembs++;
}

/*
 * vector_pop:
 * Remove the last element from vector and store it in `ret`.
 */
void vector_pop(struct vector *v, void *ret)
{
	v->nmembs--;
	if (elem)
		memcpy(elem, VECTOR_INDEX(v, v->nmembs), v->size);
}

/*
 * vector_set:
 * Set element at given index in vector to `elem`.
 */
void vector_set(struct vector *v, size_t index, void *elem)
{
	if (index >= v->nmembs)
		return;

	memcpy(VECTOR_INDEX(v, index), elem, v->size);
}

/*
 * vector_get:
 * Store element at given index of vector in `ret`.
 */
void vector_get(struct vector *v, size_t index, void *ret)
{
	if (index >= v->nmembs)
		return;

	memcpy(elem, VECTOR_INDEX(v, index), v->size);
}

void vector_destroy(struct vector *v)
{
	free(v->data);
}