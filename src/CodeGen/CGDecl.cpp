#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((NStatement *)this)->lineno)
#define getFile(p) (((NStatement *)this)->file_name)

inline void
cleanDeclInfo(DeclInfo *decl_info)
{
	delete decl_info;
}

vector<Type*>
getArgTypeList(CodeGenContext& context, ParamList params)
{
	vector<Type*> ret;
	ParamList::const_iterator param_it;
	DeclInfo *decl_info_tmp;
	Type *tmp_type;

	context.in_param_flag++;
	for (param_it = params.begin();
		 param_it != params.end(); param_it++) {
		decl_info_tmp = (*param_it)->decl.getDeclInfo(context,
													  (*param_it)->type.getType(context));
		if (decl_info_tmp) {
			tmp_type = decl_info_tmp->type;
			ret.push_back(tmp_type);
		}
		delete decl_info_tmp;
	}
	context.in_param_flag--;

	return ret;
}

static Type *
setArrayType(CodeGenContext& context, Type *elem_type, ArrayDim& array_dim, int lineno, char *file_name)
{
	Value *tmp_value;
	ConstantInt *tmp_const;
	ArrayDim::const_iterator arr_dim_di;

	for (arr_dim_di = array_dim.begin();
		 arr_dim_di != array_dim.end(); arr_dim_di++) {
		if (*arr_dim_di) { // equals []
			tmp_value = NAssignmentExpr::doAssignCast(context, (**arr_dim_di).codeGen(context),
													  Type::getInt64Ty(getGlobalContext()),
													  nullptr, lineno, file_name);
			if (tmp_const = dyn_cast<ConstantInt>(tmp_value)) {
				elem_type = ArrayType::get(elem_type, tmp_const->getZExtValue());
			} else {
				CGERR_Non_Constant_Array_Size(context);
				CGERR_setLineNum(context, lineno, file_name);
				CGERR_showAllMsg(context);
				return NULL;
			}
		} else {
			elem_type = PointerType::getUnqual(elem_type);
		}
	}

	return elem_type;
}

static Type *
setPointerType(CodeGenContext& context, Type *elem_type, int ptr_dim, int lineno)
{
	for (; ptr_dim > 0; ptr_dim--) {
		if (elem_type->isVoidTy()) {
			elem_type = context.builder->getInt8Ty();
		}
		elem_type = elem_type->getPointerTo();
	}

	return elem_type;
}

CGValue
NVariableDecl::codeGen(CodeGenContext& context)
{
	llvm::GlobalVariable *var;
	DeclaratorList::const_iterator decl_it;
	DeclInfo *decl_info_tmp;
	AllocaInst *alloc_inst;
	NIdentifier *id;
	NAssignmentExpr *assign;
	Constant *init_value = NULL;
	Type *main_type;
	Type *tmp_type;
	DeclSpecifier::const_iterator decl_spec_it;

	for (decl_spec_it = var_specifier.begin();
		 decl_spec_it != var_specifier.end(); decl_spec_it++) {
		(*decl_spec_it)->setSpecifier(specifiers);
	}

	main_type = specifiers->type->getType(context);

	if (context.currentBlock()
		&& specifiers->linkage != GlobalValue::ExternalLinkage) {
		for (decl_it = declarator_list->begin();
			 decl_it != declarator_list->end(); decl_it++) {
			decl_info_tmp = (*decl_it)->getDeclInfo(context, main_type);
			tmp_type = decl_info_tmp->type;
			if (tmp_type->isVoidTy()) {
				CGERR_Invalid_Use_Of_Void(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
				return CGValue();
			}

			alloc_inst = context.builder->CreateAlloca(tmp_type, nullptr, decl_info_tmp->id->name.c_str());
			context.getTopLocals()[decl_info_tmp->id->name] = alloc_inst;
			if (decl_info_tmp->expr) {
				id = new NIdentifier(*new string(decl_info_tmp->id->name));
				assign = new NAssignmentExpr(*id, *decl_info_tmp->expr);
				assign->codeGen(context);
				delete assign;
			}

			delete decl_info_tmp;
		}
	} else {
		for (decl_it = declarator_list->begin();
			 decl_it != declarator_list->end(); decl_it++) {
			decl_info_tmp = (*decl_it)->getDeclInfo(context, main_type);
			tmp_type = decl_info_tmp->type;

			if (isFunctionType(tmp_type)) {
				if (specifiers->linkage == GlobalValue::CommonLinkage) {
					specifiers->linkage = GlobalValue::ExternalLinkage;
				}
				Function::Create(dyn_cast<FunctionType>(tmp_type),
								 specifiers->linkage,
								 context.formatName(decl_info_tmp->id->name), context.module);
			} else {
				if (tmp_type->isVoidTy()) {
					if (specifiers->linkage == GlobalValue::ExternalLinkage) {
						tmp_type = context.builder->getInt8Ty();
					} else {
						CGERR_Invalid_Use_Of_Void(context);
						CGERR_setLineNum(context, getLine(this), getFile(this));
						CGERR_showAllMsg(context);
						return CGValue();
					}
				}

				if (specifiers->linkage != GlobalValue::ExternalLinkage) {
					init_value = Constant::getNullValue(tmp_type);
				}

				var = new GlobalVariable(*context.module, tmp_type, false,
										 specifiers->linkage,
										 init_value, context.formatName(decl_info_tmp->id->name));

				if (decl_info_tmp->expr) {
					Value *tmp_val;
					if (!context.currentBlock()) {
						CGERR_Non_Constant_Initializer(context);
						CGERR_setLineNum(context, getLine(this), getFile(this));
						CGERR_showAllMsg(context);

						context.setGlobalConstructor();
						tmp_val = NAssignmentExpr::doAssignCast(context, decl_info_tmp->expr->codeGen(context),
																tmp_type, nullptr,
																getLine(this), getFile(this));
						if (init_value = dyn_cast<Constant>(tmp_val)) {
							var->setInitializer(init_value);
						} else {
							context.builder->CreateStore(tmp_val, var);
						}
						context.popAllBlock();
					} else {
						assert(specifiers->linkage == GlobalValue::ExternalLinkage);
						tmp_val = decl_info_tmp->expr->codeGen(context);
						init_value = dyn_cast<Constant>(NAssignmentExpr::doAssignCast(context, tmp_val, tmp_type, nullptr,
																					  getLine(this), getFile(this)));
						var->setInitializer(init_value);
					}

					/* if (!(init_value = dyn_cast<Constant>(tmp_val))) {
						CGERR_External_Variable_Is_Not_Constant(context);
						CGERR_setLineNum(context, getLine(this), getFile(this));
						CGERR_showAllMsg(context);
						return CGValue();
					} */
					delete decl_info_tmp->expr;
				}

				context.getGlobals()[context.formatName(decl_info_tmp->id->name)] = var;
			}

			delete decl_info_tmp;
		}
	}

	return CGValue();
}

CGValue
NDelegateDecl::codeGen(CodeGenContext& context)
{
	FunctionType *ftype;
	DeclInfo *decl_info;

	decl_info = decl.getDeclInfo(context, type.getType(context));

	ftype = dyn_cast<FunctionType>(decl_info->type);
	context.setType(context.formatName(decl_info->id->name), ftype->getPointerTo());

	delete decl_info;

	return CGValue();
}

CGValue
NStructDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	bool isAnon = false;
	VariableList::const_iterator var_it;
	DeclaratorList::const_iterator decl_it;
	DeclInfo *decl_info_tmp;
	FieldMap field_map;
	vector<Type*> field_types;
	StructType *struct_type;
	Type *tmp_type;
	DeclSpecifier::const_iterator decl_spec_it;
	string real_name;

	if (!id.name.compare(".")) {
		isAnon = true;
		delete &id.name;
		id.name = *new string(ANON_POSTFIX);
	}
	real_name = STRUCT_PREFIX + context.formatName(id.name);

	if (context.getType(real_name)
		&& !isAnon) {
		if (context.getStruct(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Struct(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		struct_type = dyn_cast<StructType>(context.getType(real_name));
	} else if (context.getType(STRUCT_PREFIX + id.name)
			   && !isAnon) {
		real_name = STRUCT_PREFIX + id.name;
		if (context.getStruct(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Struct(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		struct_type = dyn_cast<StructType>(context.getType(real_name));
	} else {
		struct_type = StructType::create(getGlobalContext(), real_name);
		context.setType(isAnon ? "." : real_name, struct_type);
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
				decl_info_tmp = (*decl_it)->getDeclInfo(context, tmp_type);
				field_map[decl_info_tmp->id->name] = i;

				field_types.push_back(decl_info_tmp->type);

				if (decl_info_tmp->expr) {
					CGERR_Initializer_Cannot_Be_In_Struct(context);
					CGERR_setLineNum(context, getLine(*var_it), getFile(*var_it));
					CGERR_showAllMsg(context);
					return CGValue();
				}

				delete decl_info_tmp;
			}
		}
		struct_type->setBody(makeArrayRef(field_types), true);
		context.setStruct(struct_type->getStructName(), field_map);
	}

	return CGValue((Value *)struct_type);
}

CGValue
NUnionDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	bool isAnon = false;
	VariableList::const_iterator var_it;
	DeclaratorList::const_iterator decl_it;
	DeclInfo *decl_info_tmp;
	UnionFieldMap field_map;
	vector<Type*> field_types;
	StructType *union_type;
	Type *tmp_type;
	Type *main_type;
	DeclSpecifier::const_iterator decl_spec_it;
	string real_name;

	if (!id.name.compare(".")) {
		isAnon = true;
		delete &id.name;
		id.name = *new string(ANON_POSTFIX);
	}
	real_name = UNION_PREFIX + context.formatName(id.name);

	if (context.getType(real_name)
		&& !isAnon) {
		if (context.getUnion(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Union(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		union_type = dyn_cast<StructType>(context.getType(real_name));
	} else if (context.getType(UNION_PREFIX + id.name)
			   && !isAnon) {
		real_name = UNION_PREFIX + id.name;
		if (context.getUnion(real_name)
			&& fields) { // redefinition
			CGERR_Redefinition_Of_Union(context, id.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		union_type = dyn_cast<StructType>(context.getType(real_name));
	} else {
		union_type = StructType::create(getGlobalContext(), real_name);
		context.setType(isAnon ? "." : real_name, union_type);
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
				decl_info_tmp = (*decl_it)->getDeclInfo(context, main_type);
				tmp_type = decl_info_tmp->type;

				field_map[decl_info_tmp->id->name] = tmp_type;

				if (getConstantIntExprJIT(ConstantExpr::getSizeOf(tmp_type)) > max_size) {
					max_sized_type = tmp_type;
				}

				if (decl_info_tmp->expr) {
					CGERR_Initializer_Cannot_Be_In_Union(context);
					CGERR_setLineNum(context, getLine(*var_it), getFile(*ver_it));
					CGERR_showAllMsg(context);
					return CGValue();
				}
				delete decl_info_tmp;
			}
		}
		field_types.push_back(max_sized_type);
		union_type->setBody(makeArrayRef(field_types), true);
		context.setUnion(union_type->getStructName(), field_map);
	}

	return CGValue((Value *)union_type);
}

CGValue
NTypedefDecl::codeGen(CodeGenContext& context)
{
	DeclInfo *decl_info = decl.getDeclInfo(context, type.getType(context));
	Type *tmp_type = decl_info->type;

	context.setType(context.formatName(decl_info->id->name), tmp_type);

	delete decl_info;

	return CGValue();
}

CGValue
NFunctionDecl::codeGen(CodeGenContext& context)
{
	FunctionType *ftype;
	Function *function;
	BasicBlock *bblock;
	Function::arg_iterator arg_it;
	ParamList::const_iterator param_it;
	DeclSpecifier::const_iterator decl_spec_it;
	FunctionType::param_iterator param_type_it;
	Type *ret_type;
	DeclInfo *decl_info_tmp;
	DeclInfo *main_decl_info;

	for (decl_spec_it = func_specifier.begin();
		 decl_spec_it != func_specifier.end();
		 decl_spec_it++) {
		(*decl_spec_it)->setSpecifier(specifiers);
	}

	main_decl_info = decl.getDeclInfo(context, specifiers->type->getType(context));
	ret_type = dyn_cast<FunctionType>(main_decl_info->type)->getReturnType();
	ftype = dyn_cast<FunctionType>(main_decl_info->type);

	if (specifiers->linkage == GlobalValue::CommonLinkage) {
		specifiers->linkage = GlobalValue::ExternalLinkage;
	}
	if (!(function = context.module->getFunction(context.formatName(main_decl_info->id->name)))) {
		function = Function::Create(ftype, specifiers->linkage,
									context.formatName(main_decl_info->id->name), context.module);
	} else {
		for (param_type_it = ftype->param_begin(), arg_it = function->arg_begin();
			 param_type_it != ftype->param_end() && arg_it != function->arg_end();
			 param_type_it++, arg_it++) {
			if (*param_type_it == arg_it->getType()) {
				continue;
			}
			break;
		}

		if (param_type_it != ftype->param_end()
			|| arg_it != function->arg_end()) {
			CGERR_Conflicting_Type(context, main_decl_info->id->name.c_str(), param_type_it - ftype->param_begin() + 1);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	}

	if (block) {
		if (context.currentBlock()) {
			CGERR_Nesting_Function(context);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}

		if (function->begin() != function->end()) {
			CGERR_Redefinition_Of_Function(context, main_decl_info->id->name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}

		bblock = BasicBlock::Create(getGlobalContext(), "", function, 0);
		context.pushBlock(bblock);
		context.builder->SetInsertPoint(context.currentBlock());

		if (!context.formatName(main_decl_info->id->name).compare("main")) { // name is "main"
			if (isInt32Type(function->getReturnType())) {
				context.builder->CreateAlloca(function->getReturnType(), nullptr, "");
			} else {
				CGERR_Invalid_Main_Function_Return_Type(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
			}

			function->setDoesNotThrow();
			function->setHasUWTable();
			function->addFnAttr("no-frame-pointer-elim-non-leaf");
		}

		context.in_param_flag++;
		for (param_it = main_decl_info->arguments->begin(), arg_it = function->arg_begin();
			 param_it != main_decl_info->arguments->end(); param_it++, arg_it++) {
			decl_info_tmp = (*param_it)->decl.getDeclInfo(context, (*param_it)->type.getType(context));
			if (decl_info_tmp) {
				if (decl_info_tmp->id->name != "") {
					arg_it->setName(decl_info_tmp->id->name.c_str());
					AllocaInst *alloc_inst = context.builder->CreateAlloca(arg_it->getType(), nullptr, "");
					context.builder->CreateStore(arg_it, alloc_inst);
					context.getTopLocals()[decl_info_tmp->id->name] = alloc_inst;
				} else {
					CGERR_Useless_Param(context);
					CGERR_setLineNum(context, getLine(this), getFile(this));
					CGERR_showAllMsg(context);
					AllocaInst *alloc_inst = context.builder->CreateAlloca(arg_it->getType(),
																		   nullptr, "");
					context.builder->CreateStore(arg_it, alloc_inst);
				}
			}
			delete decl_info_tmp;
		}
		context.in_param_flag--;

		block->codeGen(context);
		if (!context.currentBlock()->getTerminator()) {
			if (ret_type->isVoidTy()) {
				context.builder->CreateRetVoid();
			} else {
				CGERR_Missing_Return_Statement(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
				context.builder->CreateRet(Constant::getNullValue(function->getReturnType()));
			}
		}
		context.popAllBlock();
	}

	delete main_decl_info;

	return CGValue(function);
}

CGValue
NNameSpace::codeGen(CodeGenContext& context)
{
	context.current_namespace += name + "$";

	if (block) {
		block->codeGen(context);
	}

	context.current_namespace = context.current_namespace.substr(0, context.current_namespace.length() - (name + "$").length());

	return CGValue();
}
