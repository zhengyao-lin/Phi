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

GenericValue
CodeGenContext::runCode()
{
	ExecutionEngine *ee;
	vector<GenericValue> noargs;
	GenericValue v;
	Function *main_function;
	FunctionType *main_func_type;

	ErrorMessage::tmpNote("Execute...");

	main_function = module->getFunction(MAIN_FUNCTOIN_NAME);
	if (!main_function) {
		CGERR_Missing_Main_Function(*this);
		CGERR_showAllMsg(*this);
		return v;
	}
	ee = EngineBuilder(module).create();

	main_func_type = main_function->getFunctionType();
	if (!main_func_type->getNumParams()) {
		v = ee->runFunction(main_function, noargs);
	} else {
		ErrorMessage::tmpError(
"Arguments are required in function main \
(Compile it to executable file and execute in shell)"
);
	}

	delete ee;
	return v;
}

string
CodeGenContext::getRandomString(int length)
{
	int flag, i;
	string ret_str;
	srand(clock());
  
	for (i = 0; i < length - 1; i++)
	{
		flag = rand() % 3;
		switch (flag)
		{
		    case 0:
		        ret_str += 'A' + rand() % 26;
		        break;
		    case 1:
		        ret_str += 'a' + rand() % 26;
		        break;
		    case 2:
		        ret_str += '0' + rand() % 10;
		        break;
		    default:
		        ret_str += 'x';
		        break;
		}
	}
	return ret_str;
}

void
CodeGenContext::setGlobalConstructor()
{
	if (global_constructor) {
		pushBlock(&global_constructor->back());
	} else {
		FunctionType *ftype = FunctionType::get(builder->getVoidTy(),
												ArrayRef<Type*>(), false);
		global_constructor = Function::Create(ftype, GlobalValue::ExternalLinkage,
											  ".global_ctor." + getRandomString(16), module);
		pushBlock(BasicBlock::Create(getGlobalContext(), "", global_constructor, 0));
	}
	builder->SetInsertPoint(currentBlock());
	return;
}

void
CodeGenContext::terminateGlobalConstructor()
{
	setGlobalConstructor();
	builder->CreateRetVoid();
	popAllBlock();
	return;
}

std::map<std::string, Value*>&
CodeGenContext::getTopLocals()
{
    return blocks.top()->locals;
}

void
CodeGenContext::setTopLocals(std::map<std::string, Value*> locals)
{
	if (currentBlock()) {
    	blocks.top()->locals = locals;
	}
	return;
}

std::map<std::string, Value*>
CodeGenContext::copyTopLocals()
{
	if (currentBlock()) {
    	return blocks.top()->locals;
	}
	return *new std::map<std::string, Value*>();
}

std::map<std::string, Value*>&
CodeGenContext::getGlobals()
{
    return globals;
}

BasicBlock *
CodeGenContext::getLabel(std::string name)
{
	if (labels.find(name) == labels.end()) {
		return NULL;
	}
	return labels[name];
}

FieldMap *
CodeGenContext::getStruct(std::string name)
{
	if (currentBlock()
		&& blocks.top()->structs.find(formatName(name)) != blocks.top()->structs.end()) {
		return &blocks.top()->structs[formatName(name)];
	}

	if (currentBlock()
		&& blocks.top()->structs.find(name) != blocks.top()->structs.end()) {
		return &blocks.top()->structs[name];
	}

	if (structs.find(formatName(name)) != structs.end()) {
		return &structs[formatName(name)];
	}

	if (structs.find(name) != structs.end()) {
		return &structs[name];
	}

	return NULL;
}

void
CodeGenContext::setStruct(std::string name, FieldMap map)
{
	if (currentBlock()) {
		blocks.top()->structs[name] = map;
	} else {
		structs[name] = map;
	}

	return;
}

UnionFieldMap *
CodeGenContext::getUnion(std::string name)
{
	if (currentBlock()
		&& blocks.top()->unions.find(formatName(name)) != blocks.top()->unions.end()) {
		return &blocks.top()->unions[formatName(name)];
	}

	if (currentBlock()
		&& blocks.top()->unions.find(name) != blocks.top()->unions.end()) {
		return &blocks.top()->unions[name];
	}

	if (unions.find(formatName(name)) != unions.end()) {
		return &unions[formatName(name)];
	}

	if (unions.find(name) != unions.end()) {
		return &unions[name];
	}

	return NULL;
}

void
CodeGenContext::setUnion(std::string name, UnionFieldMap map)
{
	if (currentBlock()) {
		blocks.top()->unions[name] = map;
	} else {
		unions[name] = map;
	}

	return;
}

Type *
CodeGenContext::getType(std::string name)
{
	if (currentBlock()
		&& blocks.top()->local_types.find(formatName(name)) != blocks.top()->local_types.end()) {
		return blocks.top()->local_types[formatName(name)];
	}

	if (currentBlock()
		&& blocks.top()->local_types.find(name) != blocks.top()->local_types.end()) {
		return blocks.top()->local_types[name];
	}

	if (types.find(formatName(name)) != types.end()) {
		return types[formatName(name)];
	}

	if (types.find(name) != types.end()) {
		return types[name];
	}

	return NULL;
}

void
CodeGenContext::setType(std::string name, Type *type)
{
	if (currentBlock()) {
		blocks.top()->local_types[name] = type;
	} else {
		types[name] = type;
	}

	return;
}

void
CodeGenContext::setLabel(std::string name, BasicBlock *block)
{
	labels[name] = block;
	return;
}

BasicBlock *
CodeGenContext::currentBlock()
{
	if (blocks.size()) {
    	return blocks.top()->block;
	}
	return NULL;
}

BlockLocalContext
CodeGenContext::backupLocalContext()
{
	BlockLocalContext ret;

	if (currentBlock()) {
		ret.local_types = blocks.top()->local_types;
		ret.locals = blocks.top()->locals;
		ret.structs = blocks.top()->structs;
		ret.unions = blocks.top()->unions;
	}

	return ret;
}

void
CodeGenContext::restoreLocalContext(BlockLocalContext context)
{
	if (currentBlock()) {
		blocks.top()->local_types = context.local_types;
		blocks.top()->locals = context.locals;
		blocks.top()->structs = context.structs;
		blocks.top()->unions = context.unions;
	}

	return;
}

TerminatorInst *
CodeGenContext::currentTerminator()
{
	if (blocks.size()) {
    	return blocks.top()->block->getTerminator();
	}
	return NULL;
}

bool
CodeGenContext::isLValue()
{
	return is_lvalue;
}

void
CodeGenContext::setLValue()
{
	is_lvalue = true;
	return;
}

void
CodeGenContext::resetLValue()
{
	is_lvalue = false;
	return;
}

void
CodeGenContext::pushBlock(BasicBlock *block)
{
	CodeGenBlock *newb = new CodeGenBlock();

    newb->returnValue = NULL;
    newb->block = block;
	if (currentBlock()) { // inherit locals
		BlockLocalContext context = backupLocalContext();
		blocks.push(newb);
		restoreLocalContext(context);

		return;
	}

	blocks.push(newb);
	return;
}

void
CodeGenContext::popBlock()
{
    CodeGenBlock *top = blocks.top();
    blocks.pop();
    delete top;
}

void
CodeGenContext::popAllBlock()
{
	while (currentBlock()) popBlock();
	return;
}

void
CodeGenContext::setCurrentReturnValue(Value *value)
{
    blocks.top()->returnValue = value;
}

Value *
CodeGenContext::getCurrentReturnValue()
{
    return blocks.top()->returnValue;
}
