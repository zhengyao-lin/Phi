#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "AST/Parser.h"
#include "ErrorMsg/EMCore.h"

CGValue
codeGen()
{
	CGValue val(ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 1102));
	return val;
}

int main(int argc, char **argv)
{
	Value *val = codeGen();

	return 0;
}