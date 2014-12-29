%{
/**
 *	@file	wordsort.lex
 *	
 *	@brief	A simple word sort program
 *
 *	@date	2009-01
 */
%}
%option noyywrap

%%

[\t]+	 /* ignore white space */
is |
am |
are |
were |
was |
be |
being |
been |
do |
docs |
did |
will |
would |
should |
can |
could |
has |
have |
had |
go		{ printf("%s: is a verb\n", yytext); }
[a-zA-Z]+	{ printf("%s: is not a verb\n", yytext); }
.|\n		{ ECHO; }
%%

int main(void)
{
	yylex();

	return 0;
}
