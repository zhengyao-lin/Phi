#include "stds.h"

/*typedef struct VM_tag {
	int i;
	char *msg;
} VM;

struct s {
	int i, a[10] = "hi";
};*/

union ut {
	int i;
	int j;
};

int funct()
{
	return funct();
}

int main()
{
	int i = 2;
	int array[10 + 3 - 2 - 1 * 109 / (100 + 9) + sizeof(int)];
	array[i] = 10221;
	typeof(array) array2 = array;

	printf("%d\n", array2[2]);

	printf("%lld\n", sizeof(int) << sizeof(double));
	printf("%d\n", sizeof(typeof(2.0)));

	int *arrayP = &array[1];
	printf("%d\n", arrayP[1]);

	//memset(&p, 2, sizeof(typeof(p)));
	printf("%d\n", sizeof(int));

	int usss;

	int two_dims[10][10][10];
	two_dims[0][0][0] = 10;
	int two_dims2[10][10][10] = two_dims;
	printf("%d\n", two_dims2[0][0][0]);

	bool i5 = false;
	bool i6 = true;

	if (i5 && i6) {
		printf("i5 == %d && i6 == %d\n", i5, i6);
	} else if (i5) {
		printf("only i5 == %d\n", i5);
	} else if (i6) {
		printf("only i6 == %d\n", i6);
	} else {
		printf("no!\n");
	}

	return 0;
}
