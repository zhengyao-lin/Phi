#include "Screen.h"
#include "Element.h"
#include "General.h"

namespace Elements {
	static Element *element_list = NULL;

	int drawRect2(Rectangle *rect, int x, int y)
	{
		return Screen::drawRect1(y, x,
								  y + rect->height, x + rect->width,
								  rect->material);
	}

	Rectangle *getRect(int height, int width, char material)
	{
		Rectangle *ret;

		ret = malloc(sizeof(Rectangle));
		ret->height = height;
		ret->width = width;
		ret->material = material;

		return ret;
	}

	Element *allocElement(void *node)
	{
		Element *elem;

		elem = malloc(sizeof(Element));
		elem->u.anon = node;
		elem->prev = NULL;
		elem->next = NULL;

		return elem;
	}
}
