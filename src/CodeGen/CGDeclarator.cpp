#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((Declarator *)this)->lineno)
#define getFile(p) (((Declarator *)this)->file_name)

DeclInfo *
IdentifierDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	return new DeclInfo(base_type, new NIdentifier(*new string(id.name)));
}

DeclInfo *
ArrayDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	if (base_type == NULL) return decl.getDeclInfo(context, base_type);
	Value *tmp_value;
	Type *elem_type;
	ConstantInt *tmp_const;
	ArrayDim::const_iterator arr_dim_di;

	elem_type = base_type;
	for (arr_dim_di = array_dim.begin();
		 arr_dim_di != array_dim.end(); arr_dim_di++) {
		if (*arr_dim_di
			&& !context.in_param_flag) {
			tmp_value = NAssignmentExpr::doAssignCast(context, (**arr_dim_di).codeGen(context),
													  Type::getInt64Ty(getGlobalContext()),
													  nullptr, getLine(this), getFile(this));
			if (tmp_const = dyn_cast<ConstantInt>(tmp_value)) {
				elem_type = ArrayType::get(elem_type, tmp_const->getZExtValue());
			} else {
				CGERR_Non_Constant_Array_Size(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
				return NULL;
			}
		} else if (context.in_param_flag) {
			elem_type = PointerType::getUnqual(elem_type);
		} else {
			elem_type = ArrayType::get(elem_type, 0);
		}
	}

	return decl.getDeclInfo(context, elem_type);
}

DeclInfo *
PointerDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	if (base_type == NULL) return decl.getDeclInfo(context, base_type);
	int i;
	Type *elem_type;

	elem_type = base_type;
	for (i = ptr_dim; i > 0; i--) {
		if (elem_type->isVoidTy()) {
			elem_type = context.builder->getInt8Ty();
		}
		elem_type = elem_type->getPointerTo();
	}

	return decl.getDeclInfo(context, elem_type);
}

DeclInfo *
InitDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	if (base_type == NULL) return decl.getDeclInfo(context, base_type);
	DeclInfo *ret;

	ret = decl.getDeclInfo(context, base_type);
	ret->expr = initializer;

	return ret;
}

vector<Type*>
getArgTypeList(CodeGenContext& context, ParamList params);

bool
checkParam(CodeGenContext& context, int lineno, char *file_name, vector<Type*>& arguments, ParamList& arg_nodes)
{
	vector<Type*>::const_iterator param_type_it;
	ParamList::const_iterator param_it;
	DeclInfo *decl_info_tmp;
	Type *type_tmp;

	context.in_param_flag++;
	for (param_type_it = arguments.begin(), param_it = arg_nodes.begin();
		 param_type_it != arguments.end();
		 param_type_it++, param_it++) {
		type_tmp = (*param_it)->type.getType(context);
		decl_info_tmp = (*param_it)->decl.getDeclInfo(context, type_tmp);
		if (isVoidType(*param_type_it)) { // param has void type
			if (decl_info_tmp->id) {
				CGERR_Void_Type_Param(context);
				CGERR_setLineNum(context, lineno, file_name);
				CGERR_showAllMsg(context);
				return false;
			} else if (arguments.end() - arguments.begin() > 1) {
				CGERR_Void_Should_Be_The_Only_Param(context);
				CGERR_setLineNum(context, lineno, file_name);
				CGERR_showAllMsg(context);
				return false;
			}
			param_type_it = arguments.erase(param_type_it);
			param_it = arg_nodes.erase(param_it);
			param_type_it--;
			param_it--;
		}
		delete decl_info_tmp;
	}
	context.in_param_flag--;

	return true;
}

DeclInfo *
ParamDeclarator::getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
{
	if (base_type == NULL) return decl.getDeclInfo(context, base_type);
	DeclInfo *ret;
	ParamList::const_iterator param_it;
	vector<Type*> param_vec;
	FunctionType *ftype;

	param_vec = getArgTypeList(context, arguments);
	checkParam(context, ((Declarator *)this)->lineno, ((Declarator *)this)->file_name, param_vec, arguments);
	ftype = FunctionType::get(base_type, makeArrayRef(param_vec), has_vargs);

	ret = decl.getDeclInfo(context, ftype);
	ret->arguments = &arguments;

	return ret;
}
