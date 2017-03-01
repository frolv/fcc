/*
 * gen.h
 */

#include "asg.h"

void begin_translation_unit(void);
void free_translation_unit(void);
void translate_function(const char *fname, struct graph_node *g);

void flush_to_file(char *filename);
