extern int glmake_toObject(char *file_path);
extern int system(char *cmd);
extern int strcmp(char *s1, char *s2);
extern int remove(char *file);

#define PATH "Tests/NanoX/"

void
doClean()
{
	remove("Tests/NanoX/NanoX.o");
	remove("Tests/NanoX/Screen.o");
	remove("Tests/NanoX/Element.o");
	remove("Tests/NanoX/NanoX");
	return;
}

int main(int argc, char **argv)
{
	glmake_toObject("Tests/NanoX/NanoX.f");
	glmake_toObject("Tests/NanoX/Screen.f");
	glmake_toObject("Tests/NanoX/Element.f");
	system("gcc Tests/NanoX/NanoX.o Tests/NanoX/Screen.o Tests/NanoX/Element.o -o Tests/NanoX/NanoX");

	system("Tests/NanoX/NanoX");

	doClean();
	return;
}
