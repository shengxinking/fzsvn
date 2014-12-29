/*
 *
 */
#include <ncurses.h>

int main(int argc, char *argv[])
{ 
	int ch, prev;
	FILE *fp;
	int goto_prev = FALSE, y, x;
	
	if(argc != 2)
	{
		printf("Usage: %s \n", argv[0]);
		exit(1);
	}
	fp = fopen(argv[1], "r");   
	if(fp == NULL)
	{
		perror("Cannot open input file\n");
		exit(1);
	}
	
	initscr();                 
	prev = EOF;
	while((ch = fgetc(fp)) != EOF)
	{   
		if(prev == '/' && ch == '*')   
		{   
			attron(A_REVERSE | A_BLINK);           
			goto_prev = TRUE;
		}
		if(goto_prev == TRUE) 
		{ 
			getyx(stdscr, y, x);
			move(y, x - 1);
			printw("%c%c", '/', ch); 
			ch = 'a';               
			goto_prev = FALSE;     
		}
		else 
			printw("%c", ch);
		refresh();                
		if(prev == '*' && ch == '/')
			attroff(A_REVERSE | A_BLINK);     
		prev = ch;
	}
	getch();
	endwin();                
	return 0;
}
