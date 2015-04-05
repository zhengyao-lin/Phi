#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
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
#include "IO/IOSetting.h"

using namespace std;

extern FILE *yyin;
Parser *main_parser = NULL;
CodeGenContext *global_context;
vector<string> *tmp_file_paths = NULL;

__attribute__ ((destructor))
void delete_tmp()
{
	vector<string>::const_iterator tmp_it;
	for (tmp_it = tmp_file_paths->begin();
		 tmp_it != tmp_file_paths->end(); tmp_it++) {
		if (isFileExist(*tmp_it)) {
			remove(tmp_it->c_str());
		}
	}

	return;
}

int main(int argc, char **argv)
{
	PassManager pm;
	TargetMachine::CodeGenFileType output_file_type;

	tmp_file_paths = new vector<string>();
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
