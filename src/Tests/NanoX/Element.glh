#ifndef _NANOX_H_
#define _NANOX_H_

namespace Elements {
	typedef struct Rectangle_tag {
		int height;
		int width;
		char material;
	} Rectangle;

	typedef struct Element_tag {
		union {
			void *anon;
			Rectangle *rect;
		} u;

		struct Element_tag *prev;
		struct Element_tag *next;
	} Element;

	int drawRect2(Rectangle *rect, int x, int y);

	Rectangle *getRect(int height, int width, char material);
	Element *allocElement(void *node);
}

#endif
