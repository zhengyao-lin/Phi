#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((NStatement *)this)->lineno)

static vector<Type*>
getArgTypeList(CodeGenContext& context, ParamList params)
{
	vector<Type*> ret;
	ParamList::const_iterator param_it;
	ArrayDim::const_iterator arr_dim_it;
	Type *tmp_type;

	for (param_it = params.begin();
		 param_it != params.end(); param_it++) {
		tmp_type = (**param_it).type.getType(context);

		for (arr_dim_it = (**param_it).array_dim.begin();					// NOTE: All array declaration in function parameter
			 arr_dim_it != (**param_it).array_dim.end(); arr_dim_it++) {	// will all be casted to pointer
			tmp_type = PointerType::getUnqual(tmp_type);
		}

		ret.push_back(tmp_type);
	}

	return ret;
}

static Type *
setArrayType(CodeGenContext& context, Type *elem_type, ArrayDim& array_dim, int lineno)
{
	Value *tmp_value;
	ConstantInt *tmp_const;
	ArrayDim::const_iterator arr_dim_di;

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

	return elem_type;
}

Value *
NVariableDecl::codeGen(CodeGenContext& context)
{
	llvm::GlobalVariable *var;
	DeclaratorList::const_iterator decl_it;
	AllocaInst *alloc_inst;
	NIdentifier *id;
	NAssignmentExpr *assign;
	Constant *init_value;
	Type *main_type;
	Type *tmp_type;
	DeclSpecifier::const_iterator decl_spec_it;

	for (decl_spec_it = var_specifier.begin();
		 decl_spec_it != var_specifier.end(); decl_spec_it++) {
		(*decl_spec_it)->setSpecifier(specifiers);
	}

	main_type = specifiers->type->getType(context);

	if (context.currentBlock()) {
		for (decl_it = declarator_list->begin();
			 decl_it != declarator_list->end(); decl_it++) {
			tmp_type = main_type;
			if ((**decl_it).second) {
				tmp_type = setArrayType(context, tmp_type, *(**decl_it).second,
									 getLine(this));
			}


			alloc_inst = context.builder->CreateAlloca(tmp_type, nullptr, (**decl_it).first.name.c_str());
			context.getTopLocals()[(**decl_it).first.name] = alloc_inst;
			if ((**decl_it).third) {
				id = &(**decl_it).first;
				assign = new NAssignmentExpr(*id, *(**decl_it).third);
				assign->codeGen(context);
				delete assign;
			}
		}
	} else {
		for (decl_it = declarator_list->begin();
			 decl_it != declarator_list->end(); decl_it++) {
			tmp_type = main_type;
			if ((**decl_it).second) {
				tmp_type = setArrayType(context, tmp_type, *(**decl_it).second,
									 getLine(this));
			}

			init_value = Constant::getNullValue(tmp_type);
			if ((**decl_it).third) {
				if (!(init_value = dyn_cast<Constant>(NAssignmentExpr::doAssignCast(context, (**decl_it).third->codeGen(context),
																					tmp_type, nullptr,
																					getLine(this))))) {
					CGERR_External_Variable_Is_Not_Constant(context);
					CGERR_setLineNum(context, getLine(this));
					CGERR_showAllMsg(context);
					return NULL;
				}
				delete (**decl_it).third;
			}

			var = new GlobalVariable(*context.module, tmp_type, false,
									 (specifiers->is_static ? llvm::GlobalValue::InternalLinkage
															: llvm::GlobalValue::ExternalLinkage),
									 init_value, context.formatName((**decl_it).first.name)); 
			context.getGlobals()[context.formatName((**decl_it).first.name)] = var;
		}
	}

	return NULL;
}

static bool
checkParam(CodeGenContext& context, int lineno, vector<Type*>& arguments, ParamList& arg_nodes)
{
	vector<Type*>::const_iterator param_type_it;
	ParamList::const_iterator param_it;

	for (param_type_it = arguments.begin(), param_it = arg_nodes.begin();
		 param_type_it != arguments.end();
		 param_type_it++, param_it++) {
		if (isVoidType(*param_type_it)) { // param has void type
			if ((*param_it)->id.name != "") {
				CGERR_Void_Type_Param(context);
				CGERR_setLineNum(context, lineno);
				CGERR_showAllMsg(context);
				return false;
			} else if (arguments.end() - arguments.begin() > 1) {
				CGERR_Void_Should_Be_The_Only_Param(context);
				CGERR_setLineNum(context, lineno);
				CGERR_showAllMsg(context);
				return false;
			}
			param_type_it = arguments.erase(param_type_it);
			param_it = arg_nodes.erase(param_it);
			param_type_it--;
			param_it--;
		}
	}

	return true;
}

Value*
NDelegateDecl::codeGen(CodeGenContext& context)
{
    vector<Type*> arg_types;
	FunctionType *ftype;
	Type *ret_type = type.getType(context);

	arg_types = getArgTypeList(context, arguments);
	checkParam(context, getLine(this), arg_types, arguments);

	ftype = FunctionType::get(ret_type, makeArrayRef(arg_types), has_vargs);
	context.setType(context.formatName(id.name), ftype->getPointerTo());

	return NULL;
}

Value*
NStructDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	VariableList::const_iterator var_it;
	DeclaratorList::const_iterator decl_it;
	FieldMap field_map;
	vector<Type*> field_types;
	StructType *struct_type;
	Type *tmp_type;
	DeclSpecifier::const_iterator decl_spec_it;
	string real_name = STRUCT_PREFIX + context.formatName(id.name);

	if (context.getType(real_name)) {
		if (context.getStruct(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Struct(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this));
			CGERR_showAllMsg(context);
			return NULL;
		}
		struct_type = dyn_cast<StructType>(context.getType(real_name));
	} else {
		struct_type = StructType::create(getGlobalContext(), real_name);
		context.setType(real_name, struct_type);
	}

	if (fields) {
		for (var_it = fields->begin(), i = 0;
			 var_it != fields->end(); var_it++) {
			for (decl_spec_it = (**var_it).var_specifier.begin();
				 decl_spec_it != (**var_it).var_specifier.end(); decl_spec_it++) {
				(**decl_spec_it).setSpecifier((**var_it).specifiers);
			}

			tmp_type = (**var_it).specifiers->type->getType(context);
			for (decl_it = (**var_it).declarator_list->begin();
				 decl_it != (**var_it).declarator_list->end(); decl_it++, i++) {
				field_map[(*decl_it)->first.name] = i;

				if ((**decl_it).second) {
					field_types.push_back(setArrayType(context, tmp_type,
													   *(**decl_it).second,
													   getLine(*var_it)));
				} else {
					field_types.push_back(tmp_type);
				}

				if ((**decl_it).third) {
					CGERR_Initializer_Cannot_Be_In_Struct(context);
					CGERR_setLineNum(context, getLine(*var_it));
					CGERR_showAllMsg(context);
					return NULL;
				}
			}
		}
		struct_type->setBody(makeArrayRef(field_types), true);
		context.setStruct(struct_type->getStructName(), field_map);
	}

	return (Value *)struct_type;
}

Value*
NUnionDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	VariableList::const_iterator var_it;
	DeclaratorList::const_iterator decl_it;
	UnionFieldMap field_map;
	vector<Type*> field_types;
	StructType *union_type;
	Type *tmp_type;
	Type *main_type;
	DeclSpecifier::const_iterator decl_spec_it;
	string real_name = UNION_PREFIX + context.formatName(id.name);

	if (context.getType(real_name)) {
		if (context.getUnion(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Union(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this));
			CGERR_showAllMsg(context);
			return NULL;
		}
		union_type = dyn_cast<StructType>(context.getType(real_name));
	} else {
		union_type = StructType::create(getGlobalContext(), real_name);
		context.setType(real_name, union_type);
	}

	if (fields) {
		Type *max_sized_type = NULL;
		uint64_t max_size = 0;

		for (var_it = fields->begin(), i = 0;
			 var_it != fields->end(); var_it++) {
			for (decl_spec_it = (**var_it).var_specifier.begin();
				 decl_spec_it != (**var_it).var_specifier.end();
				 decl_spec_it++) {
				(**decl_spec_it).setSpecifier((**var_it).specifiers);
			}

			main_type = (**var_it).specifiers->type->getType(context);

			for (decl_it = (**var_it).declarator_list->begin();
				 decl_it != (**var_it).declarator_list->end(); decl_it++, i++) {

				if ((**decl_it).second) {
					tmp_type = setArrayType(context, main_type,
										 *(**decl_it).second, getLine(*var_it));
				} else {
					tmp_type = main_type;
				}


				field_map[(**decl_it).first.name] = tmp_type;

				if (getConstantIntExprJIT(ConstantExpr::getSizeOf(tmp_type)) > max_size) {
					max_sized_type = tmp_type;
				}

				if ((**decl_it).third) {
					CGERR_Initializer_Cannot_Be_In_Union(context);
					CGERR_setLineNum(context, getLine(*var_it));
					CGERR_showAllMsg(context);
					return NULL;
				}
			}
		}
		field_types.push_back(max_sized_type);
		union_type->setBody(makeArrayRef(field_types), true);
		context.setUnion(union_type->getStructName(), field_map);
	}

	return (Value *)union_type;
}

Value*
NTypedefDecl::codeGen(CodeGenContext& context)
{
	Type *tmp_type = type.getType(context);

	tmp_type = setArrayType(context, tmp_type, array_dim, getLine(this));

	context.setType(context.formatName(id.name), tmp_type);

	return NULL;
}

Value*
NFunctionDecl::codeGen(CodeGenContext& context)
{
	vector<Type*> arg_types;
	FunctionType *ftype;
	Function *function;
	BasicBlock *bblock;
	Function::arg_iterator arg_it;
	ParamList::const_iterator param_it;
	DeclSpecifier::const_iterator decl_spec_it;
	vector<Type*>::const_iterator param_type_it;
	Type *ret_type;

	for (decl_spec_it = func_specifier.begin();
		 decl_spec_it != func_specifier.end();
		 decl_spec_it++) {
		(*decl_spec_it)->setSpecifier(specifiers);
	}

	ret_type = specifiers->type->getType(context);
	arg_types = getArgTypeList(context, arguments);
	checkParam(context, getLine(this), arg_types, arguments);

	ftype = FunctionType::get(ret_type, makeArrayRef(arg_types), has_vargs);

	if (!(function = context.module->getFunction(context.formatName(id.name)))) {
		function = Function::Create(ftype,
									(specifiers->is_static ? GlobalValue::InternalLinkage
														   : GlobalValue::ExternalLinkage),
									context.formatName(id.name), context.module);
	} else {
		for (param_type_it = arg_types.begin(), arg_it = function->arg_begin();
			 param_type_it != arg_types.end() && arg_it != function->arg_end();
			 param_type_it++, arg_it++) {
			if (isSameType(*param_type_it, arg_it->getType())) {
				continue;
			}
			break;
		}

		if (param_type_it != arg_types.end()
			|| arg_it != function->arg_end()) {
			CGERR_Conflicting_Type(context, id.name.c_str(), param_type_it - arg_types.begin() + 1);
			CGERR_setLineNum(context, getLine(this));
			CGERR_showAllMsg(context);
			return NULL;
		}
	}

	if (block) {
		if (context.currentBlock()) {
			CGERR_Nesting_Function(context);
			CGERR_setLineNum(context, getLine(this));
			CGERR_showAllMsg(context);
			return NULL;
		}

		bblock = BasicBlock::Create(getGlobalContext(), "", function, 0);
		context.pushBlock(bblock);
		context.builder->SetInsertPoint(context.currentBlock());

		for (param_it = arguments.begin(), arg_it = function->arg_begin();
			 param_it != arguments.end(); param_it++, arg_it++) {
			if ((*param_it)->id.name != "") {
				arg_it->setName((*param_it)->id.name.c_str());
				AllocaInst *alloc_inst = context.builder->CreateAlloca(arg_it->getType(), nullptr, "");
				context.builder->CreateStore(arg_it, alloc_inst);
				context.getTopLocals()[(*param_it)->id.name] = alloc_inst;
			} else {
				CGERR_Useless_Param(context);
				CGERR_setLineNum(context, getLine(this));
				CGERR_showAllMsg(context);
				AllocaInst *alloc_inst = context.builder->CreateAlloca(arg_it->getType(),
																	   nullptr, "");
				context.builder->CreateStore(arg_it, alloc_inst);
			}
		}

		block->codeGen(context);
		if (!context.currentBlock()->getTerminator()) {
			if (ret_type->isVoidTy()) {
				context.builder->CreateRetVoid();
			} else {
				CGERR_Missing_Return_Statement(context);
				CGERR_setLineNum(context, getLine(this));
				CGERR_showAllMsg(context);
				return NULL;
			}
		}
		context.popAllBlock();
	}

	return function;
}

Value *
NNameSpace::codeGen(CodeGenContext& context)
{
	context.current_namespace += name + ".";

	if (block) {
		block->codeGen(context);
	}

	context.current_namespace = context.current_namespace.substr(0, context.current_namespace.length() - (name + ".").length());

	return NULL;
}
