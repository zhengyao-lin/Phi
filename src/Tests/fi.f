#include "stds.h"

int fi(int n, int m)
{
	if (n == 1) {
		return m;
	}
	if (m == 1) {
		return n;
	}

	return fi(n - 1, m) + fi(n, m -1);
}

static int ip = 0;

int print_where()
{
	printf("in global!\n");
	return 0;
}

namespace sd {
	namespace subtype {
		typedef struct occ { double ppp; } occt;
		struct incomp {
			int i;
		};
	}

	char nsp_array[10];
	int print_where();

	int env2(int i)
	{
		subtype::occt obj;
		printf("%f\n", obj.ppp = 10);

		::print_where();

		return 0;
	}

	int print_where()
	{
		printf("in namespace!\n");
		return 0;
	}
}

int:64 strlen_o(char *str)
{
	int:64 ret;

	FOR_S:
		ret = 0;
	FOR_COND:
		if (*str != '\0')
		{ str++; ret++; goto FOR_COND; }

	return ret;
}

int main(void)
{
	//typedef struct occ { int iii; } occt;
	struct sd::subtype::incomp oo;

	sd::env2(10);

	sd::subtype::occt obj;
	printf("%d\n", sd::nsp_array[9] = 1000);

	{
		//struct incomp ic;
		//int ppp = ic.i;
	}
	printf("%d\n", fi(10, 10));

	char *i = "hello";
	printf("%s\n", i);

	/* do-while */
	WHILE1:
	{
		printf("%d\n", ip);
		ip = ip + 1;
	}
	if (ip < 10) {
		goto WHILE1;
	}


	printf("%d\n", ip = 120);

	{
		int i[2][1][1];

		**((i+1)[0]) = 10;
		printf("%d\n", (i+1)[0][0][0]);
	}


	{
		int i[2];

		*(i+1) = 10;
		printf("%d\n", i[1]);
	}

	{
		int i[2][2];

		**(i+1) = 111;
		printf("%d\n", i[1][0]);
	}

	{
		int i[10];
		int *a;
		a = i;
	}

	{
		void* x = 0;
		void* y = &*x;
	}

	{
		char *i = "hello, world";
		*i++ = 'b';
		*i++ = 'o';
		*i = 'b';
		i = i - 2;
		printf("%d, %c, %c, %c, %c\n", strlen_o(i), *i++, *i++, *i++, *i);
		int i = 10;
		int a = i = 20;
	}

	return 0;
}
