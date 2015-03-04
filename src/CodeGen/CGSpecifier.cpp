#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

void
NSpecifier::setSpecifier(SpecifierSet *dest)
{
	return;
}

void
NExternSpecifier::setSpecifier(SpecifierSet *dest)
{
	dest->is_static = false;
	return;
}

void
NStaticSpecifier::setSpecifier(SpecifierSet *dest)
{
	dest->is_static = true;
	return;
}

void
NTypeSpecifier::setSpecifier(SpecifierSet *dest)
{
	dest->type = &type;
	return;
}
