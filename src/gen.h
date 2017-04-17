/*
 * src/gen.h
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

#ifndef FCC_GEN_H
#define FCC_GEN_H

#include "asg.h"

void begin_translation_unit(void);
void free_translation_unit(void);
void translate_function(const char *fname,
                        struct ast_node *params,
                        struct graph_node *g);

void flush_to_file(char *filename);

#endif /* FCC_GEN_H */
