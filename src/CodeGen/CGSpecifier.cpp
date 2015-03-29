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
	dest->linkage = GlobalValue::ExternalLinkage;
	return;
}

void
NStaticSpecifier::setSpecifier(SpecifierSet *dest)
{
	dest->linkage = GlobalValue::InternalLinkage;
	return;
}

void
NTypeSpecifier::setSpecifier(SpecifierSet *dest)
{
	dest->type = &type;
	return;
}
