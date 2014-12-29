#include <ncurses.h>
#include <string.h>

int main()
{
    char	mesg[]="Just a message";
    int		row, col;

    initscr();

    getmaxyx(stdscr, row, col);

    mvprintw(row/2, (col-strlen(mesg))/2, "%s", mesg);

    mvprintw(row - 2, 0, "This screen has %d rows and %d columns\n", row, col);
    printw("Try resizeing you window(if possible) and then run this program again");
    refresh();

    getch();
    endwin();

    return 0;
}
