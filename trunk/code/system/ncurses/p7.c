#include <ncurses.h>

WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);

int main(int argc, char *argv[])
{       
	WINDOW *my_win;
	int startx, starty, width, height;
	int ch;

	initscr();                                      /* ��ʼ��������cursesģʽ */
	cbreak();                                       /* �л����ֹ���������п�����Ϣ */
	keypad(stdscr, TRUE);                   /* ������Ҫʹ��F1���ܼ� */

	height = 3;
	width = 10;
	starty = (LINES - height) / 2;  /* ���㴰������λ�õ����� */
	startx = (COLS - width) / 2;            /* ���㴰������λ�õ����� */
	printw("Press F1 to exit");
	refresh();
	my_win = create_newwin(height, width, starty, startx);

	while((ch = getch()) != KEY_F(1))
	{       
		switch(ch)
		{       
			case KEY_LEFT:
				destroy_win(my_win);
				my_win = create_newwin(height, width, starty,--startx);
				break;
			case KEY_RIGHT:
				destroy_win(my_win);
				my_win = create_newwin(height, width, starty,++startx);
				break;
			case KEY_UP:
				destroy_win(my_win);
				my_win = create_newwin(height, width, --starty,startx);
				break;
			case KEY_DOWN:
				destroy_win(my_win);
				my_win = create_newwin(height, width, ++starty,startx);
				break;  
		}
	}

	endwin();                                       /* ����cursesģʽ */
	return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx)
{       
	WINDOW *local_win;
	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);                  /* 0, 0���ַ�Ĭ�ϵ�������ʼλ�� */
	wrefresh(local_win);                    /* ˢ�´��ڻ��壬��ʾbox */
	return local_win;
}

void destroy_win(WINDOW *local_win)
{
	/* box(local_win, ' ', ' ');���ᰴ��Ԥ�ڵ�����������ڱ߿� �����ڴ��ڵ��ĸ��������²����ַ� */
//  wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	/* ����ע��9.3�� 
	 * 1. win:��ǰ�����Ĵ���
	 * 2. ls:������ʾ������߽���ַ� 
	 * 3. rs:������ʾ�����ұ߽���ַ� 
	 * 4. ts:������ʾ�����ϱ߽���ַ� 
	 * 5. bs:������ʾ�����±߽���ַ� 
	 * 6. tl:������ʾ�������Ͻǵ��ַ�
	 * 7. tr:������ʾ�������Ͻǵ��ַ�
	 * 8. bl:������ʾ�������½ǵ��ַ�
	 * 9. br:������ʾ�������½ǵ��ַ�
	 */
	wrefresh(local_win);
	delwin(local_win);
}
