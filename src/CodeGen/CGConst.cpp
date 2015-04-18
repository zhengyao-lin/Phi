#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"
#include <sstream>
#include <string.h>

CGValue
NInteger::codeGen(CodeGenContext& context)
{
	unsigned radix = (value.c_str()[0] == '0'
					   && strlen(value.c_str()) > 1 ? (value.c_str()[1] == 'x'
													   || value.c_str()[1] == 'X' ? 16 : 8)
													: 10);
	string str = value.substr(radix == 16 ? 2 : (radix == 8 ? 1 : 0 ));
	unsigned bits = APInt::getBitsNeeded(str, radix);
	bits = bits > 32 ? (bits / 32 + (bits % 32 == 0 ? 0 : 1)) * 32 : 32;

	ConstantInt *ret = ConstantInt::get(Type::getIntNTy(getGlobalContext(), bits),
										StringRef(str), radix);

	return CGValue(ret);
}

CGValue
NChar::codeGen(CodeGenContext& context)
{
	return CGValue(context.builder->getInt8(value));
}

CGValue
NBoolean::codeGen(CodeGenContext& context)
{
	return CGValue(context.builder->getInt1(value));
}

CGValue
NDouble::codeGen(CodeGenContext& context)
{
	return CGValue(ConstantFP::get(Type::getDoubleTy(getGlobalContext()),
									value));
}

CGValue
NString::codeGen(CodeGenContext& context)
{
	if (context.currentBlock()) {
		return CGValue(new GlobalVariable(*context.module,
								   llvm::ArrayType::get(Type::getInt8Ty(getGlobalContext()), strlen(value.c_str()) + 1),
								   true, GlobalValue::PrivateLinkage, 
								   ConstantDataArray::getString(getGlobalContext(), value.c_str()),
								   ".str"));
	}

	return CGValue(ConstantDataArray::getString(getGlobalContext(), value.c_str()));
}
