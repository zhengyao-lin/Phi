#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "AST/Parser.h"
#include "ErrorMsg/EMCore.h"
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/CommandLine.h>

using namespace std;

extern FILE *yyin;
Parser *main_parser = NULL;

class IOSetting
{
	bool target_asm = false;
	bool target_object = false;
	string input_file = "";
	string object_file = "";

public:
	#define ARG_OBJECT ("-o")
	#define ARG_TARGET_OBJECT ("-c")
	#define ARG_TARGET_ASM ("-S")

	enum ArgumentType {
		Unknown = 0,
		ObjectFile,
		TargetObj,
		TargetASM,
	};
	std::map<std::string, ArgumentType> ARG_MAP;

	void initMap()
	{
		ARG_MAP[ARG_OBJECT] = ObjectFile;
		ARG_MAP[ARG_TARGET_OBJECT] = TargetObj;
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
				case TargetObj:
					target_object = true;
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

	virtual ~IOSetting()
	{
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

	bool targetObj()
	{
		return target_object;
	}

	bool targetASM()
	{
		return target_asm;
	}

	bool isIROutput()
	{
		return !targetObj() && !targetASM();
	}

	string getFileName(string file)
	{
		int i;
		for (i = file.length() - 1; i >= 0; i--) {
			if (file[i] == '.') {
				break;
			}
		}
		return file.substr(0, i);
	}

	void doOutput(Module *mod)
	{
		TargetMachine::CodeGenFileType output_file_type = TargetMachine::CGFT_Null;

		if (targetObj()) {
			output_file_type = TargetMachine::CGFT_ObjectFile;
		} else {
			output_file_type = TargetMachine::CGFT_AssemblyFile;
		}

		if (isIROutput()) {
			if (getObject().empty()) {
				if (input_file.empty()) {
					object_file = "tmp.ll";
				} else {
					object_file = getFileName(string(basename(input_file.c_str()))) + ".ll";
				}
			}

			string error_msg;
			tool_output_file output_file(getObject().c_str(), error_msg, sys::fs::F_None);
			if (!error_msg.empty()) {
				cerr << error_msg << endl;
				return;
			}
			output_file.os() << *mod;
			output_file.keep();
		} else {
			string error_str;
			const Target *target = TargetRegistry::lookupTarget(
									sys::getDefaultTargetTriple(), error_str);
			if (target == NULL) {
				cout << error_str << endl;
				return;
			}
			TargetOptions target_options;
			TargetMachine *target_machine = target->createTargetMachine(
										   sys::getDefaultTargetTriple(),
										   sys::getHostCPUName(), "",
										   target_options);

			if (object_file.empty()) {
				if (input_file.empty()) {
					if (targetObj()) {
						object_file = "tmp.o";
					} else {
						object_file = "tmp.s";
					}
				} else {
					if (targetObj()) {
						object_file = getFileName(string(basename(input_file.c_str()))) + ".o";
					} else {
						object_file = getFileName(string(basename(input_file.c_str()))) + ".s";
					}
				}
			}

			string error_str2;
			tool_output_file ouput_tool(object_file.c_str(), error_str2, sys::fs::F_Excl);
			if (!error_str2.empty()) {
				cout << error_str2 << endl;
				return;
			}
			PassManager pass_m;
			//passManager.add(dataLayout);
			formatted_raw_ostream fos(ouput_tool.os());
			target_machine->addPassesToEmitFile(pass_m, fos, output_file_type);
			pass_m.run(*mod);
			ouput_tool.keep();
		}
	}
};

int llc_main(int argc, char **argv);

CodeGenContext *global_context;

int main(int argc, char **argv)
{
	PassManager pm;
	TargetMachine::CodeGenFileType output_file_type;

	global_context = new CodeGenContext();
	main_parser = new Parser();
	IOSetting *settings = new IOSetting(argc, argv);
	settings->applySetting();

	main_parser->startParse(yyin);

	InitializeNativeTarget();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmPrinters();
	InitializeAllAsmParsers();

	main_parser->generateAllDecl(*global_context);
	global_context->generateCode(*main_parser->getAST());
	delete main_parser;

	settings->doOutput(global_context->module);

	global_context->module->dump();
	global_context->runCode();

	llvm_shutdown();
	delete global_context;
	delete settings;

	return 0;
}
