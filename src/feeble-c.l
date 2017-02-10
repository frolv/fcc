/* Feeble C lex specification */

%option reentrant bison-bridge
%option yylineno

%{
#include "fcc.h"
#include "parse.h"

#define YY_NO_INPUT
#define YY_NO_UNPUT
%}

nonzero_digit   [1-9]
digit           {nonzero_digit}|0
octal_digit     [0-7]
hex_digit       {digit}|[a-fA-F]
hex_prefix      0[xX]
unsigned        [uU]
letter          [a-zA-Z]
id_nondigit     {letter}|"_"
escape_sequence "\\n"|"\\t"|"\\'"|"\\\""|"\\\\"|"\\0"
string_char     [^\n"]|{escape_sequence}
char_char       [^\n'\\]|{escape_sequence}
line_comment    "//"

%x C_COMMENT

%%

break                                   { return TOKEN_BREAK; }
continue                                { return TOKEN_CONTINUE; }
char                                    { return TOKEN_CHAR; }
do                                      { return TOKEN_DO; }
else                                    { return TOKEN_ELSE; }
for                                     { return TOKEN_FOR; }
if                                      { return TOKEN_IF; }
int                                     { return TOKEN_INT; }
signed                                  { return TOKEN_SIGNED; }
sizeof                                  { return TOKEN_SIZEOF; }
struct                                  { return TOKEN_STRUCT; }
unsigned                                { return TOKEN_UNSIGNED; }
void                                    { return TOKEN_VOID; }
while                                   { return TOKEN_WHILE; }
return                                  { return TOKEN_RETURN; }

"["                                     { return '['; }
"]"                                     { return ']'; }
"{"                                     { return '{'; }
"}"                                     { return '}'; }
"("                                     { return '('; }
")"                                     { return ')'; }

"."                                     { return '.'; }
"&"                                     { return '&'; }
"*"                                     { return '*'; }
"+"                                     { return '+'; }
"-"                                     { return '-'; }
"~"                                     { return '~'; }
"!"                                     { return '!'; }
"/"                                     { return '/'; }
"%"                                     { return '%'; }
"<"                                     { return '<'; }
">"                                     { return '>'; }
"^"                                     { return '^'; }
"|"                                     { return '|'; }
"="                                     { return '='; }
","                                     { return ','; }
";"                                     { return ';'; }

"->"                                    { return TOKEN_PTR; }
"<<"                                    { return TOKEN_LSHIFT; }
">>"                                    { return TOKEN_RSHIFT; }
"<="                                    { return TOKEN_LE; }
">="                                    { return TOKEN_GE; }
"=="                                    { return TOKEN_EQ; }
"!="                                    { return TOKEN_NEQ; }
"&&"                                    { return TOKEN_AND; }
"||"                                    { return TOKEN_OR; }
"++"                                    { return TOKEN_INC; }
"--"                                    { return TOKEN_DEC; }

{id_nondigit}({id_nondigit}|{digit})*   { return TOKEN_ID; }
{hex_prefix}{hex_digit}*{unsigned}?     { return TOKEN_CONSTANT; }
{nonzero_digit}{digit}*{unsigned}?      { return TOKEN_CONSTANT; }
0{octal_digit}*{unsigned}?              { return TOKEN_CONSTANT; }
\"{string_char}*\"                      { return TOKEN_STRLIT; }
'{char_char}'                           { return TOKEN_CONSTANT; }
[ \t\n\r]                               { /* skip whitespace */ }
"/*"                                    { BEGIN(C_COMMENT); }
<C_COMMENT>"*/"                         { BEGIN(INITIAL); }
<C_COMMENT>.|\n                         { }
{line_comment}.*                        { }
.                                       { fprintf(stderr, "unrecognized character: %s\n", yytext); exit(1); }

%%
