#include "stds.h"

int tt() { return 0; }

char getch() // unbuffered
{
	char c;
	return (read(0, &c, 1) == 1 ? c : -1);
}

int main()
{
	//getch();
	int p = 10;
	int i = (1, p++, p--);

	for (i = 0; i <= 10; i++, p--) {
		//continue;
		printf("%d, %d, %d\n", i, p, i + p);
	}

	char *ptr[10];

	*(ptr+1) = "index-1";
	*&ptr[0] = "index-0";

	printf("%s, %s\n", ptr[0], ptr[1]);

	//if (0) {
		//int in_scope = 0;
	//}
	//printf("%d\n", in_scope);
	int bits = 12;
	bits = bits > 32 ? (bits / 32 + (bits % 32 == 0 ? 0 : 1)) * 32 : 32;
	printf("%d\n", bits);

	return 0;
}
