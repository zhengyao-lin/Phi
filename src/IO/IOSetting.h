#ifndef _IOSETTING_H_
#define _IOSETTING_H_

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../CodeGen/CGAST.h"
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

#define ARG_OBJECT ("-o")
#define ARG_TARGET_OBJECT ("-c")
#define ARG_TARGET_ASM ("-s")
#define ARG_TARGET_IR ("-S")
#define ARG_TARGET_EXE ("-e")

using namespace std;
using namespace llvm;

extern vector<string> *tmp_file_paths;

inline bool
isFileExist(string path)
{
	FILE *fp = fopen(path.c_str(), "r");
	if (!fp) {
		return false;
	}
	fclose(fp);
	return true;
}

class IOSetting
{
	bool target_asm = false;
	bool target_ir = false;
	bool target_object = false;
	bool target_exe = false;
	string input_file = "";
	string object_file = "";

public:

	enum ArgumentType {
		Unknown = 0,
		ObjectFile,
		TargetObj,
		TargetASM,
		TargetIR,
		TargetExe
	};
	std::map<std::string, ArgumentType> ARG_MAP;

	void initMap();

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
				case TargetIR:
					target_ir = true;
					break;
				case TargetExe:
					target_exe = true;
					break;
				default: // input file
					input_file = argv[i];
					break;
			}
		}
	}

	virtual ~IOSetting()
	{
		/*vector<string>::const_iterator tmp_it;
		for (tmp_it = tmp_file_paths->begin();
			 tmp_it != tmp_file_paths->end(); tmp_it++) {
			if (isFileExist(*tmp_it)) {
				remove(tmp_it->c_str());
			}
		}*/
	}

	ArgumentType getArg(char *arg);

	string getRandomString(int length);

	string getTempFilePath();

	string doPreprocess(string file_path);

	void applySetting();

	bool hasInput();
	bool hasObject();
	string getObject();
	bool targetObj();
	bool targetASM();
	bool targetIR();
	bool targetExe();
	bool isIROutput();

	string getFileName(string file);
	string getFilePath(string file);

	void doOutput(Module *mod);
};

#endif
