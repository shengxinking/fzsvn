/*
 * write by jbug
 */

#include <ncurses.h>
#include <string.h>

int main()
{
    char	    msg[] = "Enter a string: ";
    char	    str[80];
    int		    row, col;

    initscr();
    getmaxyx(stdscr, row, col);
    mvprintw(row/2, (col - strlen(msg))/2, "%s", msg);

    getstr(str);
    mvprintw(LINES - 2, 0, "You entered: %s", str);
    getch();
    endwin();

    return 0;
}
