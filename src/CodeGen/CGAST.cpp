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
}

GenericValue CodeGenContext::runCode()
{
	ExecutionEngine *ee;
	vector<GenericValue> noargs;
	GenericValue v;
	Function *mainFunction;

	std::cout << "Running code...\n";

	mainFunction = module->getFunction(MAIN_FUNCTOIN_NAME);
	ee = EngineBuilder(module).create();
	v = ee->runFunction(mainFunction, noargs);

	delete ee;

	return v;
}

Value*
NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	Value *last = NULL;

	for (it = statements.begin(); it != statements.end(); it++) {
		last = (**it).codeGen(context);
	}

	return last;
}
