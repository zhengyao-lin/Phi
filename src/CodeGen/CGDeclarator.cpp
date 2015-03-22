#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((Declarator *)this)->lineno)

DeclInfo *
IdentifierDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	return new DeclInfo(base_type, id);
}

DeclInfo *
ArrayDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	DeclInfo *ret;
	Value *tmp_value;
	Type *elem_type;
	ConstantInt *tmp_const;
	ArrayDim::const_iterator arr_dim_di;

	ret = decl.getDeclInfo(context, base_type);
	elem_type = ret->type;
	for (arr_dim_di = array_dim.begin();
		 arr_dim_di != array_dim.end(); arr_dim_di++) {
		if (*arr_dim_di) { // equals []
			tmp_value = NAssignmentExpr::doAssignCast(context, (**arr_dim_di).codeGen(context),
													  Type::getInt64Ty(getGlobalContext()),
													  nullptr, lineno);
			if (tmp_const = dyn_cast<ConstantInt>(tmp_value)) {
				elem_type = ArrayType::get(elem_type, tmp_const->getZExtValue());
			} else {
				CGERR_Non_Constant_Array_Size(context);
				CGERR_setLineNum(context, lineno);
				CGERR_showAllMsg(context);
				return NULL;
			}
		} else {
			elem_type = PointerType::getUnqual(elem_type);
		}
	}
	ret->type = elem_type;

	return ret;
}

DeclInfo *
PointerDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	int i;
	DeclInfo *ret;
	Type *elem_type;

	ret = decl.getDeclInfo(context, base_type);
	elem_type = ret->type;
	for (i = ptr_dim; i > 0; i--) {
		if (elem_type->isVoidTy()) {
			elem_type = context.builder->getInt8Ty();
		}
		elem_type = elem_type->getPointerTo();
	}
	ret->type = elem_type;

	return ret;
}

DeclInfo *
InitDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	DeclInfo *ret;

	ret = decl.getDeclInfo(context, base_type);
	ret->expr = initializer;

	return ret;
}
