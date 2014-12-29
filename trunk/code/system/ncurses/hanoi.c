/*
 *
 */

#include <curses.h>
#include <string.h>
#include <unistd.h>

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1

#define NPEGS           3
#define MINTILES            3
#define MAXTILES            6
#define DEFAULTTILES        7
#define TOPLINE         6
#define BASELINE            16
#define STATUSLINE      (LINES-3)
#define LEFTPEG         19
#define MIDPEG          39
#define RIGHTPEG            59

#define LENTOIND(x)     (((x)-1)/2)
#define OTHER(a,b)      (3-((a)+(b)))

struct Peg {
    size_t Length[MAXTILES];
    int Count;
};

static struct Peg Pegs[NPEGS];
static int PegPos[] = 
{
    LEFTPEG, 
    MIDPEG, 
    RIGHTPEG
};
static int TileColour[] =
{
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
};
static int NMoves = 0;

static void InitTiles(int NTiles);
static void DisplayTiles(void);
static void MakeMove(int From, int To);
static void AutoMove(int From, int To, int Num);
static void Usage(void);
static int Solved(int NumTiles);
static int GetMove(int *From, int *To);
static int InvalidMove(int From, int To);

int main(int argc, char **argv)
{
    int NTiles, FromCol, ToCol,opt;
    unsigned char AutoFlag = 0;

    if(argc<2 || argc>3) { 
	Usage();
	exit(0);
    }
    
    while( (opt = getopt(argc,argv,"a:n:h")) != -1)
	switch (opt)
	{
	    case 'n':
		NTiles = atoi(optarg);
		if (NTiles > MAXTILES || NTiles < MINTILES)
		{   
		    fprintf(stderr, "步数从 %d 自 %d之间\n", MINTILES, MAXTILES);
		    return EXIT_FAILURE;
		}
		AutoFlag = FALSE;
		break;
	    case 'a':
		NTiles = atoi(optarg);
		if (NTiles > MAXTILES || NTiles < MINTILES)
		{       
		    fprintf(stderr, "步数从 %d 自 %d之间\n", MINTILES, MAXTILES);
		    return EXIT_FAILURE;
		}
		AutoFlag = TRUE;
		break;
	    case 'h':
	    case '?':
	    case ':':
		Usage();
		exit(0);
		break;
	}
    
    initscr();
    if (has_colors())
    {
	int i;
	int bg = COLOR_BLACK;
	start_color();
	if (use_default_colors() == OK)
	    bg = -1;
	for (i = 0; i < 9; i++)
	    init_pair(i + 1, bg, TileColour[i]);
    }
    cbreak();

    if (LINES < 24 || COLS < 80)
    {
	endwin();
	fprintf(stderr, "当前小于 24x80 \n");
	return EXIT_FAILURE;
    }
    if (AutoFlag)
    {
	curs_set(0);
	leaveok(stdscr, TRUE);
    }

    InitTiles(NTiles);
    DisplayTiles();
    if (AutoFlag) {
	do {
	    noecho();
	    AutoMove(0, 2, NTiles);
	} while (!Solved(NTiles));
	sleep(2);
    } 
    else {
	echo();
	for (;;) {
	    if (GetMove(&FromCol, &ToCol))
		break;
	    if (InvalidMove(FromCol, ToCol)) {
		mvaddstr(STATUSLINE, 0, "移 动 无 效 !!");
		refresh();
		beep();
		sleep(2);
		continue;
	    }
	    MakeMove(FromCol, ToCol);
	    if (Solved(NTiles)) {
		mvprintw(STATUSLINE, 0, "恭喜!! 恭喜!! 你用 %d 步赢了", NMoves);
		refresh();
		sleep(5);
		break;
	    }
	}
    }
    endwin();
    return EXIT_SUCCESS;
}

    static int
InvalidMove(int From, int To)
{
    if (From >= NPEGS)
	return TRUE;
    if (From < 0)
	return TRUE;
    if (To >= NPEGS)
	return TRUE;
    if (To < 0)
	return TRUE;
    if (From == To)
	return TRUE;
    if (!Pegs[From].Count)
	return TRUE;
    if (Pegs[To].Count &&
	    Pegs[From].Length[Pegs[From].Count - 1] >
	    Pegs[To].Length[Pegs[To].Count - 1])
	return TRUE;
    return FALSE;
}

    static void
InitTiles(int NTiles)
{
    int Size, SlotNo;

    for (Size = NTiles * 2 + 1, SlotNo = 0; Size >= 3; Size -= 2)
	Pegs[0].Length[SlotNo++] = Size;

    Pegs[0].Count = NTiles;
    Pegs[1].Count = 0;
    Pegs[2].Count = 0;
}

    static void
DisplayTiles(void)
{
    int Line, peg, SlotNo;
    char TileBuf[BUFSIZ];

    erase();

    init_pair(20,COLOR_MAGENTA,COLOR_BLACK);
    attrset(COLOR_PAIR(20)|A_BOLD);
    mvaddstr(1, 25,"汉     诺     塔     游     戏");
    attrset(A_NORMAL);

    mvprintw(18, 30, "当前步数 : ");
    init_pair(21,COLOR_RED,COLOR_BLACK);
    attrset(COLOR_PAIR(21)|A_BOLD);
    mvprintw(18,41,"%d",NMoves);
    attrset(A_NORMAL);

    attrset(A_REVERSE);
    mvaddstr(BASELINE, 8,
	    "                                                               ");
    for (Line = TOPLINE; Line < BASELINE; Line++) {
	mvaddch(Line, LEFTPEG, ' ');
	mvaddch(Line, MIDPEG, ' ');
	mvaddch(Line, RIGHTPEG, ' ');
    }
    mvaddch(BASELINE, LEFTPEG, '1');
    mvaddch(BASELINE, MIDPEG, '2');
    mvaddch(BASELINE, RIGHTPEG, '3');
    attrset(A_NORMAL);


    for (peg = 0; peg < NPEGS; peg++) {
	for (SlotNo = 0; SlotNo < Pegs[peg].Count; SlotNo++) {
	    memset(TileBuf, ' ', Pegs[peg].Length[SlotNo]);
	    TileBuf[Pegs[peg].Length[SlotNo]] = '\0';
	    if (has_colors())
		attrset(COLOR_PAIR(LENTOIND(Pegs[peg].Length[SlotNo])));
	    else
		attrset(A_REVERSE);
	    mvaddstr(BASELINE - (SlotNo + 1),
		    (int) (PegPos[peg] - Pegs[peg].Length[SlotNo] / 2),
		    TileBuf);
	}
    }
    attrset(A_NORMAL);
    refresh();
}

    static int
GetMove(int *From, int *To)
{

    attrset(A_REVERSE);
    mvaddstr(LINES-1, 20,"<Q>/<q> 退出       <1>-<3> 移动");
    attrset(A_NORMAL);

    mvaddstr(STATUSLINE, 0, "下一步: 从 ");
    clrtoeol();
    refresh();
    *From = getch();
    if ((*From == 'q')||(*From == 'Q') )return TRUE;
    *From -= ('0' + 1);
    addstr(" 到 ");
    clrtoeol();
    refresh();

    *To = getch();
    if ((*To == 'q') || (*To == 'Q'))return TRUE;
    *To -= ('0' + 1);
    refresh();
    napms(500);

    move(STATUSLINE, 0);
    clrtoeol();
    refresh();
    return FALSE;
}

    static void
MakeMove(int From, int To)
{
    Pegs[From].Count--;
    Pegs[To].Length[Pegs[To].Count] = Pegs[From].Length[Pegs[From].Count];
    Pegs[To].Count++;
    NMoves++;
    DisplayTiles();
}

    static void
AutoMove(int From, int To, int Num)
{
    if (Num == 1) {
	MakeMove(From, To);
	napms(500);
	return;
    }
    AutoMove(From, OTHER(From, To), Num - 1);
    MakeMove(From, To);
    napms(500);
    AutoMove(OTHER(From, To), To, Num - 1);
}

    static int
Solved(int NumTiles)
{
    int i;

    for (i = 1; i < NPEGS; i++)
	if (Pegs[i].Count == NumTiles)
	    return TRUE;
    return FALSE;
}

    static void
Usage()
{
  
}
/*----------------the end-----------------*/
