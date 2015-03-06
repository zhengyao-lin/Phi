#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"
#include <sstream>
#include <string.h>

Value*
NInteger::codeGen(CodeGenContext& context)
{
	unsigned radix = (value.c_str()[0] == '0'
					   && strlen(value.c_str()) > 1 ? (value.c_str()[1] == 'x'
													   || value.c_str()[1] == 'X' ? 16 : 8)
													: 10);
	StringRef str(value.substr(radix == 16 ? 2 : (radix == 8 ? 1 : 0 )));
	unsigned bits = APInt::getBitsNeeded(str, radix); 
	APInt apint(bits, str, radix);

	return Constant::getIntegerValue(Type::getIntNTy(getGlobalContext(), bits),
									  apint);
}

Value*
NChar::codeGen(CodeGenContext& context)
{
	return context.builder->getInt8(value);
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
	if (context.currentBlock()) {
		return new GlobalVariable(*context.module,
								   llvm::ArrayType::get(Type::getInt8Ty(getGlobalContext()), strlen(value.c_str()) + 1),
								   true, GlobalValue::PrivateLinkage, 
								   ConstantDataArray::getString(getGlobalContext(), value.c_str()),
								   ".str");
	}

	return ConstantDataArray::getString(getGlobalContext(), value.c_str());
}
