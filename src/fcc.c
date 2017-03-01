/*
 * fcc - the feeble C compiler
 */

#include <stdio.h>
#include <string.h>

#include "fcc.h"
#include "gen.h"
#include "parse.h"
#include "scan.h"
#include "symtab.h"

char *fcc_filename;
yyscan_t fcc_scanner;

void output_filename(void);

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
	begin_translation_unit();

	yyparse(fcc_scanner);
	output_filename();

	free_translation_unit();
	yylex_destroy(fcc_scanner);

	return 0;
}

/* temp */
void output_filename(void)
{
	char *file, *dot, *s;

	file = strdup(fcc_filename);

	dot = strrchr(file, '.') + 1;
	*dot = 'S';

	s = strrchr(file, '/');
	s = s ? s + 1 : file;

	flush_to_file(s);
	free(file);
}
