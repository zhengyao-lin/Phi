#include "stds.h"
delegate static int ppp();

typedef struct ChainTable_tag {
	char * name;
	struct ChainTable_tag *next;
} ChainTable;

ChainTable *
creatTable(char * name)
{
	ChainTable *ret;

	ret = malloc(sizeof(ChainTable));
	ret->name = name;
	ret->next = 0;

	return ret;
}

void
chainTable(ChainTable *last, char * name)
{
	last->next = creatTable(name);
	return;
}

int:1024
hugeInteger(int:1024 i1024)
{ // it will get the first 100 nums
	i1024 = i1024 * 2;
	return i1024;
}

int main()
{
	ChainTable *last;
	ChainTable *header = creatTable("I'm");
	last = header;

	chainTable(last, "Jack");
	last = last->next;
	printf("%s\n", header->next->name);

	free(header);
	free(last);

	int:1024 hugeI = 2015201520152015201520152015201520152015201520152015;
	hugeInteger(hugeI);
	printf("%lld\n", 999999999);
	printf("%d\n", true >> false);

	int *i;
	i = malloc(sizeof(int) * 20);

	i[19] = 123;
	printf("%d\n", i[19]);

	free(i);

	return 0;
}
