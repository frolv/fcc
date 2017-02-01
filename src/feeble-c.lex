/*
 * Feeble C lex specification.
 * Alexei Frolov
 */

%option noyywrap
%option outfile="scan.c" header-file="scan.h"

%{
%}
nonzero_digit   [1-9]
digit           {nonzero_digit}|0
octal_digit     [0-7]
hex_digit       {digit}|[a-fA-F]
hex_prefix      0[xX]
unsigned        [uU]
letter          [a-zA-Z]
id_nondigit     {letter}|"_"
keyword         break|continue|char|do|else|for|if|int|long|signed|sizeof|struct|unsigned|void|while|return
escape_sequence "\\n"|"\\t"|"\\'"|"\\\""|"\\\\"|"\\0"
string_char     [^\n"]|{escape_sequence}
char_char       [^\n'\\]|{escape_sequence}
operator        "."|"->"|"&"|"*"|"+"|"-"|"~"|"!"|"/"|"%"|"<<"|">>"|"<"|">"|"<="|">="|"=="|"!="|"^"|"|"|"&&"|"||"|"="|","
punctuator      [{}\[\]();]
line_comment    "//"

%x C_COMMENT
%x C_LINE_COMMENT

%%

{keyword}                               { printf("Keyword:\t%s\n", yytext); }
{id_nondigit}({id_nondigit}|{digit})*   { printf("Identifier:\t%s\n", yytext); }
{hex_prefix}{hex_digit}*{unsigned}?     { printf("Number:\t\t%s\n", yytext); }
{nonzero_digit}*{unsigned}?             { printf("Number:\t\t%s\n", yytext); }
0{octal_digit}*{unsigned}?              { printf("Number:\t\t%s\n", yytext); }
{operator}                              { printf("Operator:\t%s\n", yytext); }
{punctuator}                            { printf("Punctuator:\t%s\n", yytext); }
\"{string_char}*\"                      { printf("String Lit:\t%s\n", yytext); }
'{char_char}'                           { printf("Char Lit:\t%s\n", yytext); }
[ \t\n\r]                               { /* skip whitespace */ }
"/*"                                    { BEGIN(C_COMMENT); }
<C_COMMENT>"*/"                         { BEGIN(INITIAL); }
<C_COMMENT>.|\n                         { }
{line_comment}                          { BEGIN(C_LINE_COMMENT); }
<C_LINE_COMMENT>\n                      { BEGIN(INITIAL); }
<C_LINE_COMMENT>.                       { }
.                                       { fprintf(stderr, "unrecognized character: %s\n", yytext); exit(1); }

%%

int main(int argc, char **argv)
{
	yylex();
}
