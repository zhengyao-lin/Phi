#include "stds.h"
char global1[20] = "adsss";

typedef struct TT_tag {
	int i;
	int p;
	struct TT_tag *o;
} TT;

typedef struct ChainTable_tag {
	int Data;
	struct ChainTable_tag *next;
} ChainTable;

typedef int (*printf_t)(char *str, ...);

char *
strlen(char *str)
{
	int ret = 0;
	while(str[ret] != '\0') ret++;
	return ret;
}

char *
reverse(char *str)
{
	int length = strlen(str);
	int i = length - 1;
	char tmp;

	while (i > length / 2 - 1) {
		tmp = str[i];
		str[i] = str[length - i - 1];
		str[length - i - 1] = tmp;
		i--;
	}

	return str;
}

char *castBinary(int:1024 integer)
{
	int length = 0;
	int:1 bit;
	char *tmp = malloc(sizeof(char) * 1024);

	while (integer > 0 || length < 1024) {
		bit = integer;
		sprintf(tmp + length, "%d", bit);
		integer = integer >> 1;
		length++;
	}
	printf("%s\n", reverse(tmp));

	return tmp;
}

int main()
{
	ChainTable header;
	ChainTable *next;

	printf_t tt = printf;
	tt("hi\n");

	int:32 i = 0;

	while (i < 10)
	{
		if (i < 10)
			HERE: printf("%d\n", i++);
		printf("out!\n");
		i++;i++;i++;i++;
	}
	//goto HERE;

	castBinary(102402220221333333333333333333333333333333333333333333333333334444444444444444444444444444444);

	return 0;
}
