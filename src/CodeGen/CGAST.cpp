#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

using namespace std;
#define MAIN_FUNCTOIN_NAME ("main")

void
CodeGenContext::generateCode(NBlock& AST)
{
	AST.codeGen(*this);
	terminateGlobalConstructor();
	appendToGlobalCtors(*module, global_constructor, 65535);
	return;
}

GenericValue CodeGenContext::runCode()
{
	ExecutionEngine *ee;
	vector<GenericValue> noargs;
	GenericValue v;
	Function *mainFunction;

	std::cout << "Running code...\n";

	mainFunction = module->getFunction(MAIN_FUNCTOIN_NAME);
	if (!mainFunction) {
		CGERR_Missing_Main_Function(*this);
		CGERR_showAllMsg(*this);
		return v;
	}
	ee = EngineBuilder(module).create();
	v = ee->runFunction(mainFunction, noargs);

	delete ee;

	return v;
}
