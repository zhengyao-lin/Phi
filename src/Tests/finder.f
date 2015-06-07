#include "stds.h"
#define abs(a) ((a) < 0 ? -(a) : (a))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

extern int strncmp(char *str1, char *str2, int n);

bool
findSubStr(char *sub, char *dist, int len)
{
	int dist_len = strlen(dist);
	int i;

	for (i = 0; i < dist_len - len; i++) {
		if (!strncmp(sub, &dist[i], len)) {
			return true;
		}
	}

	return false;
}

double
getNameCompared(char *name, char *dist)
{
	int i, j;
	int index, len, max_len;
	int name_len = strlen(name);
	int dist_len = strlen(dist);
	int min_len = min(name_len, dist_len);
	double weight = 100.0; // max [100.0], min [no limit]
	double point;

	if (min_len <= 0) return 0.0;

	//test 1
	if (name_len != dist_len) { // don't have a same length --> -0.5 * distinction
		weight -= (5.0 + 3.0) * (abs(name_len - dist_len) / min_len) * 2;
	}
	
	//test 2
	point = 3.0;
	for (i = 0, j = 0, len = 0, max_len = len;
		 i < name_len && j < dist_len; i++, j++) {
		if (name[i] != dist[i]) {
			weight -= point;
			len = 0;
			max_len = max(max_len, len);
			point *= 1.01 - (max_len / min_len) * 0.1;
		} else {
			len++;
			max_len = max(max_len, len);
			weight += point;
			point *= 0.99 - (max_len / min_len) * 0.1;
		}
	}

	//test 3
	point = 3.0;
	for (index = 0; index < min_len - 1; index++) {
		for (len = 1; len <= min_len - index; len++) {
			if (strncmp(&name[index], &dist[index], len)) {
				if (findSubStr(&name[index], dist, len)) {
					weight -= point;
				} else {
					weight -= point + 1.0;
				}
				point *= 1.01 - (len / min_len) * 0.1;
			} else {
				point *= 0.99 - (len / min_len) * 0.1;
			}
		}
	}

	return weight;
}

#define BUFFER_SIZE 1000
typedef struct StringList_tag {
	double weight;
	char *content;
} StringList;

int main()
{
	int i, count;
	char buffer[BUFFER_SIZE];
	StringList list[BUFFER_SIZE];

	printf("%.12lf\n", getNameCompared("hipperotomonssjshasquippedaliohobia", "hippopotomonstrosesquippedaliophobia"));

#if 0
	scanf("%d", &count);

	for (i = 0; i < count; i++) {
		scanf("%s", &buffer[0]);
		list[i].weight = 0.0;
		list[i].content = strdup(buffer);
	}

	printf("Compare to: ");
	scanf("%s", &buffer[0]);
	for (i = 0; i < count; i++) {
		list[i].weight = getNameCompared(list[i].content, buffer);
		printf("%lf\n", list[i].weight);
	}
#endif

	return 0;
}
