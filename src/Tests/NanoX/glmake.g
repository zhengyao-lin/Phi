extern int glmake_toObject(char *file_path);
extern int system(char *cmd);
extern int strcmp(char *s1, char *s2);
extern int remove(char *file);

void
doClean()
{
	remove("NanoX.o");
	remove("Screen.o");
	remove("Element.o");
	remove("NanoX");
	return;
}

int main(int argc, char **argv)
{
	glmake_toObject("NanoX.g");
	glmake_toObject("Screen.g");
	glmake_toObject("Element.g");
	system("gcc NanoX.o Screen.o Element.o -o NanoX");

	system("./NanoX");

	doClean();
	return;
}
