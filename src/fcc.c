/*
 * fcc - the feeble C compiler
 */

#include <stdio.h>

#include "fcc.h"
#include "parse.h"
#include "scan.h"

char *fcc_filename;

int main(int argc, char **argv)
{
	FILE *f;
	yyscan_t scanner;

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILE\n", argv[0]);
		return 1;
	}

	if (!(f = fopen(argv[1], "r"))) {
		perror(argv[1]);
		return 1;
	}

	fcc_filename = argv[1];
	yylex_init(&scanner);
	yyset_in(f, scanner);

	yyparse(scanner);

	yylex_destroy(scanner);

	return 0;
}
