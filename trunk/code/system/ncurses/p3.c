/*
 * write by jbug
 */

#include <ncurses.h>

int main()
{
    int		ch;

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    printw("Type any charater to see it in bold\n");

    ch = getch();

    if (ch == KEY_F(1))
	printw("F1 key pressed");
    else {
	printw("The pressed key is ");
	attron(A_BOLD);
	printw("%c", ch);
	attroff(A_BOLD);
    }

    refresh();
    getch();
    endwin();
    return 0;
}

