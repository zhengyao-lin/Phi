#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "AST/Parser.h"
#include "ErrorMsg/EMCore.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;

extern FILE *yyin;
Parser *main_parser = NULL;

class IOSetting
{
	bool target_asm = false;
	bool just_compile = false;
	string input_file = "";
	string object_file = "";

public:
	#define ARG_OBJECT ("-o")
	#define ARG_JUST_COMPILE ("-c")
	#define ARG_TARGET_ASM ("-S")

	enum ArgumentType {
		Unknown = 0,
		ObjectFile,
		JustCompile,
		TargetASM,
	};
	std::map<std::string, ArgumentType> ARG_MAP;

	void initMap()
	{
		ARG_MAP[ARG_OBJECT] = ObjectFile;
		ARG_MAP[ARG_JUST_COMPILE] = JustCompile;
		ARG_MAP[ARG_TARGET_ASM] = TargetASM;
		return;
	}

	IOSetting(int argc, char **argv)
	{
		int i;

		initMap();
		for (i = 1; i < argc; i++) {
			switch (getArg(argv[i])) {
				case ObjectFile:
					object_file = argv[i + 1];
					i++;
					break;
				case JustCompile:
					just_compile = true;
					break;
				case TargetASM:
					target_asm = true;
					break;
				default: // input file
					input_file = argv[i];
					break;
			}
		}
	}

	ArgumentType getArg(char *arg)
	{
		if (ARG_MAP.find(arg) != ARG_MAP.end()) {
			return ARG_MAP[arg];
		}

		return Unknown;
	}

	void applySetting()
	{
		extern FILE *yyin;
		if (hasInput()) {
			yyin = fopen(input_file.c_str(), "r");
			if (!yyin) {
				ErrorMessage::tmpError("Cannot find source file: " + input_file);
			}
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

	bool targetASM()
	{
		return target_asm;
	}
};

int llc_main(int argc, char **argv);

int main(int argc, char **argv)
{
	PassManager pm;
	CodeGenContext context;

	main_parser = new Parser();
	IOSetting *settings = new IOSetting(argc, argv);
	settings->applySetting();

	main_parser->startParse(yyin);

	InitializeNativeTarget();

	main_parser->generateAllDecl(context);
	context.generateCode(*main_parser->getAST());
	delete main_parser;

	if (settings->hasObject()) {
		raw_fd_ostream rfo(settings->getObject().c_str(),
						   *new string(""), sys::fs::F_RW);
		pm.add(createPrintModulePass(rfo));
		pm.run(*context.module);
		rfo.close();
	}

	if (!settings->justCompile()) {
		context.module->dump();
		context.runCode();
	}

	if (settings->targetASM() && settings->hasObject()) {
		char *llc_argv[] = { "llc", strdup(settings->getObject().c_str()) };
		llc_main(2, llc_argv);
	}

	llvm_shutdown();

	return 0;
}
