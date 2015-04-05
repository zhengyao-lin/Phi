#include "IO/IOSetting.h"
#include "AST/Parser.h"
#include <llvm/Support/ManagedStatic.h>

using namespace llvm;

extern FILE *yyin;
extern Parser *main_parser;
extern CodeGenContext *global_context;

extern "C" int
glmake_toObject(char *file_path)
{
	char *args[] = { "", file_path, "-c" };
	IOSetting *settings;
	PassManager pm;
	TargetMachine::CodeGenFileType output_file_type;

	Parser *parser_backup = main_parser;
	CodeGenContext *context_backup = global_context;

	global_context = new CodeGenContext();
	main_parser = new Parser();
	settings = new IOSetting(3, args);
	settings->applySetting();

	main_parser->startParse(yyin);

	main_parser->generateAllDecl(*global_context);
	global_context->generateCode(*main_parser->getAST());
	delete main_parser;

	settings->doOutput(global_context->module);

	delete global_context;
	delete settings;

	main_parser = parser_backup;
	global_context = context_backup;

	return 0;
}
