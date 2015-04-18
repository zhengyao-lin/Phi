#include "Screen.h"
#include "Element.h"
#include "General.h"

namespace Screen {
	static int _cols = 0;
	static int _lines = 0;
	static char **_screen = 0;

	int getLines() { return _lines; }
	int getCols() { return _cols; }

	static int getScreenCols()
	{
		struct winsize win_size;
		ioctl(STDIN_FILENO, TIOCGWINSZ, &win_size);
		return win_size.ws_col;
	}

	static int getScreenLines()
	{
		struct winsize win_size;
		ioctl(STDIN_FILENO, TIOCGWINSZ, &win_size);
		return win_size.ws_row;
	}

	int initTerminal()
	{
		int i = 0;
		_cols = getScreenCols();
		_lines = getScreenLines();
		_screen = malloc(sizeof(char *) * _lines);

		while (i < _lines) {
			_screen[i] = malloc(sizeof(char) * _cols);
			memset(_screen[i], ' ', sizeof(char) * _cols);
			i++;
		}

		return 0;
	}

	int refreshScreen()
	{
		int x = 0;
		int y = 0;

		while (y < _lines) {
			printf("\n");
			while (x < _cols) {
				printf("%c", _screen[y][x]);
				x++;
			}
			x = 0;
			y++;
		}

		return 0;
	}

	int setBackground(char *color)
	{
		return printf(color);
	}

	int setForeground(char *color)
	{
		return printf(color);
	}

	int setDefault()
	{
		int i = 0;
		printf(BG_DEFAULT);

		while (i < _lines) {
			memset(_screen[i], ' ', sizeof(char) * _cols);
			i++;
		}
		refreshScreen();

		return 0;
	}

	int printStr(char *str, int line, int col)
	{
		memcpy(&_screen[line][col], str, strlen(str));
		refreshScreen();

		return 0;
	}

	int printHCent(char *str, int line) // horizontal
	{
		return printStr(str, line, (_cols - strlen(str)) / 2);
	}

	int printVCent(char *str, int col) // vertical
	{
		return printStr(str, _lines / 2 - 1, col);
	}

	int drawRect1(int line1, int col1, int line2, int col2, char material)
	{
		int x1 = col1;
		int x2 = col2;
		int y1 = line1;
		int y2 = line2;

		while (x1 != x2) {
			_screen[y1][x1] = material;
			_screen[y2][x1] = material;
			if (x1 > x2) {
				x1--;
			} else {
				x1++;
			}
		}
		x1 = col1;

		while (y1 != y2) {
			_screen[y1][x1] = material;
			_screen[y1][x2] = material;
			if (y1 > y2) {
				y1--;
			} else {
				y1++;
			}
		}
		_screen[y2][x2] = material;

		refreshScreen();

		return 0;
	}
}
