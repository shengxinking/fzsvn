/*
 * write by jbug
 */


#include <ncurses.h> 

int main() 
{ 
    initscr(); /* ��ʼ��������NCURSESģʽ */ 
    printw("Hello World !!!"); /* ��������Ļ�ϴ�ӡHello, World!!! */ 
    refresh(); /* ��������Ļ�ϵ�����д����ʾ���ϣ���ˢ��*/ 
//    getch(); /* �ȴ��û����� */ 
    endwin(); /* �˳�NCURSESģʽ */ 
    return 0; 
}


