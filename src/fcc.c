/*
 * fcc - the feeble C compiler
 */

#include <stdio.h>

#include "fcc.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"

char *fcc_filename;

int main(int argc, char **argv)
{
	FILE *f;
	yyscan_t scanner;

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
	yylex_init(&scanner);
	yyset_in(f, scanner);

	symtab_init();
	yyparse(scanner);

	yylex_destroy(scanner);

	return 0;
}
