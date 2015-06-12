#include "stds.h"

struct a {
	union { int i; } p1;
	union { int j; } p2;
	union { int j; } p3;
};

union anon1 {
	union {
		float i1;
	};
};

int main(int argc, char *argv[])
{
	struct a p;
	union anon1 aa;

	printf("%s\n", argc > 1 ? argv[1] : "");
	return 0;
}
