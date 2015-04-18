#ifndef _SCREEN_H_
#define _SCREEN_H_

#define BG_DEFAULT "\033[0m"
#define BG_IVORY "\033[47m"
#define FG_BLACK "\033[30m"

namespace Screen {
	int getLines();
	int getCols();
	int initTerminal();
	int refreshScreen();

	int setBackground(char *color);
	int setForeground(char *color);
	int setDefault();

	int printStr(char *str, int line, int col);
	int printHCent(char *str, int line); // horizontal
	int printVCent(char *str, int col); // vertical

	int drawRect1(int line1, int col1, int line2, int col2, char material);
}

#endif
