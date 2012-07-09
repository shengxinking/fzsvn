%{

/**
 *	@file	echo.lex
 *	
 *	@brief	echo each line when read a line.
 *
 *	@date	2009-01-22
 */

%}
%option noyywrap

%%

.|\n		ECHO;

%%

int main(void)
{

	yylex();

	return 0;
}
