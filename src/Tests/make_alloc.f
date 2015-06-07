extern int glmake_toObject(char *file_path);
extern int system(char *cmd);
extern int strcmp(char *s1, char *s2);
extern int remove(char *file);

void
doClean()
{
	remove("Tests/NanoX/Screen.o");
	remove("Tests/NanoX/Element.o");
	remove("Tests/alloc.o");
	remove("Tests/alloc");
	return;
}

int main(int argc, char **argv)
{
	glmake_toObject("Tests/NanoX/Screen.f");
	glmake_toObject("Tests/NanoX/Element.f");
	glmake_toObject("Tests/alloc.f");
	system("gcc Tests/alloc.o Tests/NanoX/Screen.o Tests/NanoX/Element.o -o Tests/alloc");

	system("Tests/alloc");

	doClean();
	return;
}
