/*
 * fcc - the feeble C compiler
 */

#include <stdio.h>

#include "fcc.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"

char *fcc_filename;
yyscan_t fcc_scanner;

int main(int argc, char **argv)
{
	FILE *f;

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILE\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(argv[1], "r"))) {
		perror(argv[1]);
		return 1;
	}

	fcc_filename = f == stdin ? "<stdin>" : argv[1];
	yylex_init(&fcc_scanner);
	yyset_in(f, fcc_scanner);

	symtab_init();
	yyparse(fcc_scanner);

	yylex_destroy(fcc_scanner);

	return 0;
}
