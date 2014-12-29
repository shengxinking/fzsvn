/**
 *	@file	test.lex
 *
 *	@brief	A test lexi
 *
 */

%{
	int wordCount = 0;
%}

chars [A-Za-z\_\'\.\"]
numbers ([0-9])+
delim [" "\n\t]
whitespace {delim}+
words {chars}+
%%

{words} { wordCount++; /* increase the word count by one */ }
{whitespace} {}
{numbers} {}
%%

void main(void)
{
	yylex();
	printf("Number of words: %d\n", wordCount);

	return 0;
}

int yywrap()
{
	return 1;
}
