#include "stds.h"

double atof(char *s)
{
	double val, power;
	int i, sign;

	for (i = 0; isspace(s[i]); i++);
	sign = (s[i] == '-') ? -1 : 1;
	if (s[i] == '+' || s[i] == '-') i++;

	for (val = 0.0; isdigit(s[i]); i++)
		val = 10.0 * val + (s[i] - '0');

	if (s[i] == '.') i++;

	for (power = 1.0; isdigit(s[i]); i++) {
		val = 10.0 * val + (s[i] - '0');
		power *= 10.0;
	}

	return sign * val / power;
}

Tremend
atot(char *s)
{
	Tremend val, i;
	int sign;

	for (i = 0; isspace(s[i]); i++);
	sign = (s[i] == '-') ? -1 : 1;
	if (s[i] == '+' || s[i] == '-') i++;

	for (val = 0; isdigit(s[i]); i++)
		val = 10 * val + (s[i] - '0');

	return sign * val;
}

int main()
{
	printf("%lld\n", atot("1024512256128643216"));
	return 0;
}
