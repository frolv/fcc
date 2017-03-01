/*
 * fcc - the feeble C compiler
 */

#ifndef FCC_FCC_H
#define FCC_FCC_H

extern char *fcc_filename;
extern void *fcc_scanner;

#define ALIGN(x, a)             __ALIGN_MASK(x, (a) - 1)
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))
#define ALIGNED(x, a)           (((x) & ((a) - 1)) == 0)

#endif /* FCC_FCC_H */
