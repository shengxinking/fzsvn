/*
 *
 */

#include <ncurses.h>
#include <string.h>

int main()
{
	MEVENT		event;
	int			ch;

	initscr();
	cbreak();
	noecho();

	mousemask(ALL_MOUSE_EVENTS, NULL);

	while (1) {
		ch = getch();
		if (ch == KEY_MOUSE) {	
			if (getmouse(&event) == OK) {
				mvprintw(0, 0, "mouse id=%d\nmouse position: x = %d, y = %d, z=%d\n ", 
						event.id, event.x, event.y, event.z);
				refresh();
			}
			else {
				mvprintw(0, 0, "You input %c\n", ch);
				refresh();
			}
			break;
		}
		else if (ch == 'q')
			break;
		else {
			mvprintw(0, 0, "You input %c\n", ch);
			refresh();
		}
	}

	endwin();
	return 0;
}


