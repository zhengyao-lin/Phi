int ret_four() { return 4; }

int i;

__attribute__ ((constructor))
void h()
{
	i = ret_four();
}

int main()
{
	printf("%d\n", i);
}
