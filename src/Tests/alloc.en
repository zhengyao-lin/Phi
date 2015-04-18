#include "stds.h"
#include "NanoX/Screen.h"
#include "NanoX/Element.h"
#include "NanoX/General.h"
#define ALLOCSIZE 1000000

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

static char allocbuf[ALLOCSIZE];
static char *allocp = allocbuf;

void *alloc(int n)
{
	if (allocbuf + ALLOCSIZE - allocp >= n) {
		allocp += n;
		return allocp - n;
	}
	return NULL;
}

void afree(void *p)
{
	if (p >= allocbuf && p < allocbuf + ALLOCSIZE)
		allocp = p;
	return;
}



typedef struct Chain_tag { struct Chain_tag *next; } Chain;

int main()
{
	Chain *c = alloc(sizeof(Chain));
	printf("%d\n", c->next);
	afree(c);

	Screen::initTerminal();
	Screen::setBackground(BG_IVORY);
	Screen::setForeground(FG_BLACK);
	Screen::refreshScreen();

	stp();
	Screen::setDefault();
	
	return 0;
}
