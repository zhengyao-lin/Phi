#include "Screen.h"
#include "Element.h"
#include "General.h"

void
set_keypress(void)
{
	system("stty raw -echo");
	return;
}

void
reset_keypress(void)
{
	system("stty -raw echo");
	return;
}

void stp()
{
	char c;

	set_keypress();
	while (c = getchar()) {
		if (c == 'q') {
			goto BREAK;
		}
		reset_keypress();
		Screen::refreshScreen();
		set_keypress();
	}
	BREAK:
	reset_keypress();

	return;
}

int main()
{
	Elements::Rectangle *rect;

	Screen::initTerminal();

	Screen::setBackground(BG_IVORY);
	Screen::setForeground(FG_BLACK);

	Screen::printStr("Welcome", 5, 7);
	Screen::printStr("Press 'q' to exit", 6, 7);
	Elements::drawRect2(Elements::getRect(Screen::getLines() - 1,
										  Screen::getCols() - 1, ' '),
						0, 0);
	Elements::drawRect2(Elements::getRect(10, 30, '\''), 2, 2);
	Screen::refreshScreen();
	stp();

	Screen::setDefault();

	return 0;
}
