/*
 * src/fcc.h
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

#ifndef FCC_FCC_H
#define FCC_FCC_H

extern char *fcc_filename;
extern void *fcc_scanner;

#define ALIGN(x, a)             __ALIGN_MASK(x, (a) - 1)
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))
#define ALIGNED(x, a)           (((x) & ((a) - 1)) == 0)

#endif /* FCC_FCC_H */
