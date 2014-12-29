/*
 *
 *
 *
 */

%{
#include "y.tab.h"
#include <stdio.h>
#include <string.h>

extern char * yylval;
%}

char [A-Za-z]
num [0-9]
eq [=]
name {char}+
age {num}+
%%

{name} { yylval = strdup(yytest); return NAME;}
{eq} { return EQ; }


