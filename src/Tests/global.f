#include "stds.h"

#define ret_four() (4)

#define min(x,y) (x < y ? x : y)

int i = ret_four();

int main()
{
	printf("%d, %d\n", i, min(20, 10));
}
