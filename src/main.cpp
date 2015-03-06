#include <iostream>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "ErrorMsg/EMCore.h"
#include "llvm/Support/ManagedStatic.h"

using namespace std;

extern int yyparse();
extern int yylex_destroy();
extern NBlock* AST;

void createCoreFunctions(CodeGenContext& context);

int main(int argc, char **argv)
{
	yyparse();
	yylex_destroy();

	InitializeNativeTarget();
	CodeGenContext context;
	context.generateCode(*AST);
	delete AST;
	context.module->dump();
	context.runCode();

	llvm_shutdown();

	return 0;
}
