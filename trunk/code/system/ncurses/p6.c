/*
 *
 */

#include <ncurses.h>

int main(int argc, char *argv[])
{       
	initscr();                              /* ���� curses ģʽ */
	start_color();                  /* ������ɫ������ */

	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	printw("A Big string which i didn't care to type fully ");
	mvchgat(0, 0, -1, A_BLINK, 1, NULL);    
	/* 
	 *��һ���������������˺�����ʼ��λ��
	 *�����������Ǳ��ı����ε��ַ�����Ŀ��-1��ʾһ����
	 *���ĸ������Ǳ��ı��������
	 *�������������ɫ��������ɫ�����Ѿ���init_pair()�б����Ի���
	 *����ã���ʾ��ʹ����ɫ��
	 *���һ������NULL��ûʲô�ر�����塣
	 */
	refresh();
	getch();
	endwin();                               /* ����cursesģʽ */
	return 0;
}
