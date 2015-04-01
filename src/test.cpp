#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include "CodeGen/CGAST.h"
#include "AST/Node.h"
#include "AST/Parser.h"
#include "ErrorMsg/EMCore.h"

int main(int argc, char **argv)
{
	CGValue val;
	val.addKind(LeftValueKind, RightValueKind, MultiValueKind);
	printf("%d\n", val.hasKind<LeftValueKind>());
	CGValue val2 = val;
	printf("%d\n", val2.hasKind<LeftValueKind>());

	return 0;
}
