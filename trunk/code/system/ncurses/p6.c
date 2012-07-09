/*
 *
 */

#include <ncurses.h>

int main(int argc, char *argv[])
{       
	initscr();                              /* 进入 curses 模式 */
	start_color();                  /* 开启颜色管理功能 */

	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	printw("A Big string which i didn't care to type fully ");
	mvchgat(0, 0, -1, A_BLINK, 1, NULL);    
	/* 
	 *第一、二个参数表明了函数开始的位置
	 *第三个参数是被改变修饰的字符的数目，-1表示一整行
	 *第四个参数是被改变的修饰名
	 *第五个参数是颜色索引。颜色索引已经在init_pair()中被初试化了
	 *如果用０表示不使用颜色。
	 *最后一个总是NULL，没什么特别的意义。
	 */
	refresh();
	getch();
	endwin();                               /* 结束curses模式 */
	return 0;
}
