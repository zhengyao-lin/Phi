#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

Value*
NInteger::codeGen(CodeGenContext& context)
{
	StringRef str(value);
	APInt apint(context.currentBitWidth, str, 10);
	long long integer = atol(value);

	if (context.currentBitWidth) {
		return Constant::getIntegerValue(Type::getIntNTy(getGlobalContext(), context.currentBitWidth),
										  apint);
	}

	return context.builder->getInt32(integer);
}

Value*
NBoolean::codeGen(CodeGenContext& context)
{
	return context.builder->getInt1(value);
}

Value*
NDouble::codeGen(CodeGenContext& context)
{
	return ConstantFP::get(Type::getDoubleTy(getGlobalContext()),
							value);
}

Value*
NString::codeGen(CodeGenContext& context)
{
	StringRef ref(value.c_str());
	return context.builder->CreateGlobalString(ref, ".str");
}
