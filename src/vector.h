/*
 * vector.h
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

void vector_append(struct vector *v, void *elem);
int vector_pop(struct vector *v, void *ret);

int vector_set(struct vector *v, size_t index, void *elem);
int vector_get(struct vector *v, size_t index, void *ret);

#endif /* FCC_VECTOR_H */
