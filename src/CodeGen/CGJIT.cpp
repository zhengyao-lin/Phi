#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

static uint64_t __ret_value = 0;

#define RETURN_CALLEE ("__Return_Callee")
extern "C"
void
__Return_Callee(uint64_t value)
{
	__ret_value = value;
	return;
}


// ***getSizeOfTypeJIT***
// Create module like this:
//
// extern void __Return_Callee(Int64 value);
// int main () {
//     __Return_Callee(sizeof(?type?));
// }
uint64_t
getConstantIntExprJIT(Constant *const_expr)
{
	Module *module = new Module("ConstantExpr.JIT", getGlobalContext());
	Function *mainFunc;
	Function *callee;
	FunctionType *ftype;
	FunctionType *ftype2;
	BasicBlock *bblock;
	ExecutionEngine *ee;
	vector<GenericValue> noargs;
	Type *argType[] = {
		Type::getInt64Ty(getGlobalContext())
	};
	Value *args[] = {
		(Value *)const_expr
	};

	ftype = FunctionType::get(Type::getInt32Ty(getGlobalContext()), false);
	mainFunc = Function::Create(ftype,
								GlobalValue::InternalLinkage,
								"main", module);

	ftype2 = FunctionType::get(Type::getVoidTy(getGlobalContext()),
							   makeArrayRef(argType), false);
	callee = Function::Create(ftype2, GlobalValue::ExternalLinkage,
							  RETURN_CALLEE, module);

	bblock = BasicBlock::Create(getGlobalContext(), "entrypoint", mainFunc, 0);
	CallInst::Create(callee, args, "", bblock);
	ReturnInst::Create(getGlobalContext(), ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0), bblock);

	ee = ExecutionEngine::create(module);
	ee->runFunction(mainFunc, noargs);

	delete ee;

	return __ret_value;
}
