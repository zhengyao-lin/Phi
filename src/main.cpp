#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "ErrorMsg/EMCore.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;

static FILE *fp;
extern FILE *yyin;
extern int yyparse();
extern int yylex_destroy();
extern NBlock* AST;
void createCoreFunctions(CodeGenContext& context);

class IOSetting
{
	bool just_compile = false;
	string input_file = "";
	string object_file = "";

public:
	#define ARG_OBJECT ("-o")
	#define ARG_JUST_COMPILE ("-c")

	enum ArgumentType {
		Unknown = 0,
		ObjectFile,
		JustCompile,
	};

	IOSetting(int argc, char **argv)
	{
		int i;

		for (i = 1; i < argc; i++) {
			switch (getArg(argv[i])) {
				case ObjectFile:
					object_file = argv[i + 1];
					i++;
					break;
				case JustCompile:
					just_compile = true;
					break;
				default: // input file
					input_file = argv[i];
					break;
			}
		}
	}

	ArgumentType getArg(char *arg)
	{
		if (!strcmp(arg, ARG_OBJECT)) {
			return ObjectFile;
		} else if (!strcmp(arg, ARG_JUST_COMPILE)) {
			return JustCompile;
		}

		return Unknown;
	}

	void applySetting()
	{
		extern FILE *yyin;
		if (hasInput()) {
			yyin = fopen(input_file.c_str(), "r");
		} else {
			yyin = stdin;
		}
	}

	bool hasInput()
	{
		return input_file != "";
	}

	bool hasObject()
	{
		return object_file != "";
	}

	string getObject()
	{
		return object_file;
	}

	bool justCompile()
	{
		return just_compile;
	}
};

int main(int argc, char **argv)
{
	PassManager pm;
	IOSetting *settings = new IOSetting(argc, argv);
	settings->applySetting();

	yyparse();
	yylex_destroy();

	InitializeNativeTarget();
	CodeGenContext context;
	context.generateCode(*AST);
	delete AST;

	if (settings->hasObject()) {
		raw_fd_ostream rfo(settings->getObject().c_str(),
						   *new string(""), sys::fs::F_RW);
		pm.add(createPrintModulePass(rfo));
		pm.run(*context.module);
	}

	if (!settings->justCompile()) {
		context.module->dump();
		context.runCode();
	}

	llvm_shutdown();

	return 0;
}
