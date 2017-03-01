/*
 * types.c
 */

#include "types.h"

static size_t sizes[] = {
	[TYPE_INT] = 4,
	[TYPE_CHAR] = 1
};

size_t type_size(unsigned int type_flags)
{
	int type;

	type = FLAGS_TYPE(type_flags);
	if (type == TYPE_STRLIT || type == TYPE_VOID)
		return 0;
	else if (FLAGS_IS_PTR(type_flags))
	         return 4;
	else
		return sizes[type];
}
