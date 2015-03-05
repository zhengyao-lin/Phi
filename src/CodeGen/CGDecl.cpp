#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

static vector<Type*>
getArgTypeList(CodeGenContext& context, ParamList args)
{
	vector<Type*> ret;
	ParamList::const_iterator it;
	ArrayDim::const_iterator di;
	Type *tmp_type;

	for (it = args.begin();
		 it != args.end(); it++) {
		tmp_type = (**it).type.getType(context);

		for (di = (**it).array_dim.begin();			// NOTE: All array declaration in function parameter
			 di != (**it).array_dim.end(); di++) {	// will all be casted to pointer
			tmp_type = PointerType::getUnqual(tmp_type);
		}

		ret.push_back(tmp_type);
	}

	return ret;
}

static Type *
setArrayType(CodeGenContext& context, Type *T, ArrayDim& array_dim, int line_number)
{
	Value *tmp_value;
	ConstantInt *tmp_const;
	ArrayDim::const_iterator di;

	for (di = array_dim.begin();
		 di != array_dim.end(); di++) {
		if (*di) { // equals []
			tmp_value = NAssignmentExpr::doAssignCast(context, (**di).codeGen(context),
													  Type::getInt64Ty(getGlobalContext()), nullptr, line_number);
			if (tmp_const = dyn_cast<ConstantInt>(tmp_value)) {
				T = ArrayType::get(T, tmp_const->getZExtValue());
			} else {
				CGERR_Non_Constant_Array_Size(context);
				CGERR_setLineNum(context, line_number);
				CGERR_showAllMsg(context);
			}
		} else {
			T = PointerType::getUnqual(T);
		}
	}

	return T;
}

Value *
NVariableDecl::codeGen(CodeGenContext& context)
{
	llvm::GlobalVariable *var;
	DeclaratorList::const_iterator it;
	AllocaInst *alloc;
	NIdentifier *id;
	NAssignmentExpr *assign;
	Constant *init_value;
	Type *T;
	Type *tmp_T;
	DeclSpecifier::const_iterator si;

	for (si = var_specifier.begin();
		 si != var_specifier.end(); si++) {
		(*si)->setSpecifier(specifiers);
	}

	T = specifiers->type->getType(context);

	if (context.currentBlock()) {
		for (it = declarator_list->begin();
			 it != declarator_list->end(); it++) {
			tmp_T = T;
			if ((**it).second) {
				tmp_T = setArrayType(context, tmp_T, *(**it).second,
									 dyn_cast<NStatement>(this)->line_number);
			}


			alloc = context.builder->CreateAlloca(tmp_T, nullptr, (**it).first.c_str());
			context.getTopLocals()[(**it).first] = alloc;
			if ((**it).third) {
				id = new NIdentifier((**it).first);
				assign = new NAssignmentExpr(*id, *(**it).third);
				assign->codeGen(context);
				delete assign;
			}
		}
	} else {
		for (it = declarator_list->begin();
			 it != declarator_list->end(); it++) {
			tmp_T = T;
			if ((**it).second) {
				tmp_T = setArrayType(context, tmp_T, *(**it).second,
									 dyn_cast<NStatement>(this)->line_number);
			}

			init_value = Constant::getNullValue(tmp_T);
			if ((**it).third) {
				if (!(init_value = dyn_cast<Constant>(NAssignmentExpr::doAssignCast(context, (**it).third->codeGen(context),
																					tmp_T, nullptr,
																					line_number)))) {
					CGERR_External_Variable_Is_Not_Constant(context);
					CGERR_setLineNum(context, dyn_cast<NStatement>(this)->line_number);
					CGERR_showAllMsg(context);
				}
				delete (**it).third;
			}

			var = new GlobalVariable(*context.module, tmp_T, false,
									 (specifiers->is_static ? llvm::GlobalValue::InternalLinkage
															: llvm::GlobalValue::ExternalLinkage),
									 init_value, (**it).first); 
			context.getGlobals()[(**it).first] = var;
		}
	}

	return NULL;
}

Value*
NDelegateDecl::codeGen(CodeGenContext& context)
{
    vector<Type*> argTypes;
	FunctionType *ftype;
	Type *ret_type = type.getType(context);

	argTypes = getArgTypeList(context, arguments);

	ftype = FunctionType::get(ret_type, makeArrayRef(argTypes), has_vargs);
	context.types[id.name] = ftype->getPointerTo();

	return NULL;
}

Value*
NStructDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	VariableList::const_iterator vi;
	DeclaratorList::const_iterator di;
	FieldMap field_map;
	vector<Type*> field_types;
	StructType *struct_type;
	StringRef struct_name(STRUCT_PREFIX + id.name);
	Type *T;
	DeclSpecifier::const_iterator si;

	struct_type = StructType::create(getGlobalContext(), struct_name);
	context.types[STRUCT_PREFIX + id.name] = struct_type;

	for (vi = fields.begin(), i = 0;
		 vi != fields.end(); vi++) {
		for (si = (**vi).var_specifier.begin();
			 si != (**vi).var_specifier.end(); si++) {
			(**si).setSpecifier((**vi).specifiers);
		}

		T = (**vi).specifiers->type->getType(context);
		for (di = (**vi).declarator_list->begin();
			 di != (**vi).declarator_list->end(); di++, i++) {
			field_map[(*di)->first] = i;

			if ((**di).second) {
				field_types.push_back(setArrayType(context, T,
												   *(**di).second, ((NStatement)(**vi)).line_number));
			} else {
				field_types.push_back(T);
			}

			if ((**di).third) {
				CGERR_Initializer_Cannot_Be_In_Struct(context);
				CGERR_setLineNum(context, ((NStatement)(**vi)).line_number);
				CGERR_showAllMsg(context);
			}
		}
	}
	struct_type->setBody(makeArrayRef(field_types), true);
	context.structs[STRUCT_PREFIX + id.name] = field_map;

	return (Value *)struct_type;
}

Value*
NUnionDecl::codeGen(CodeGenContext& context)
{
	unsigned i;
	VariableList::const_iterator vi;
	DeclaratorList::const_iterator di;
	UnionFieldMap field_map;
	vector<Type*> field_types;
	StructType *union_type;
	StringRef union_name(UNION_PREFIX + id.name);
	Type *tmp_T;
	Type *T;
	DeclSpecifier::const_iterator si;

	Type *max_sized_type = NULL;
	uint64_t max_size = 0;

	union_type = StructType::create(getGlobalContext(), union_name);
	context.types[UNION_PREFIX + id.name] = union_type;

	for (vi = fields.begin(), i = 0;
		 vi != fields.end(); vi++) {
		for (si = (**vi).var_specifier.begin();
			 si != (**vi).var_specifier.end(); si++) {
			(**si).setSpecifier((**vi).specifiers);
		}

		T = (**vi).specifiers->type->getType(context);

		for (di = (**vi).declarator_list->begin();
			 di != (**vi).declarator_list->end(); di++, i++) {

			if ((**di).second) {
				tmp_T = setArrayType(context, T,
									 *(**di).second, ((NStatement)(**vi)).line_number);
			} else {
				tmp_T = T;
			}


			field_map[(**di).first] = tmp_T;

			if (getConstantIntExprJIT(ConstantExpr::getSizeOf(tmp_T)) > max_size) {
				max_sized_type = tmp_T;
			}

			if ((**di).third) {
				CGERR_Initializer_Cannot_Be_In_Union(context);
				CGERR_setLineNum(context, ((NStatement)(**vi)).line_number);
				CGERR_showAllMsg(context);
			}
		}
	}
	field_types.push_back(max_sized_type);
	union_type->setBody(makeArrayRef(field_types), true);
	context.unions[UNION_PREFIX + id.name] = field_map;

	return (Value *)union_type;
}

Value*
NTypedefDecl::codeGen(CodeGenContext& context)
{
	Type *T = type.getType(context);

	T = setArrayType(context, T, array_dim, this->line_number);

	context.types[id.name] = T;

	return NULL;
}

Value*
NFunctionDecl::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	FunctionType *ftype;
	Function *function;
	BasicBlock *bblock;
	Function::arg_iterator ai;
	ParamList::const_iterator it;
	DeclSpecifier::const_iterator si;
	StoreInst *inst;
	Type *ret_type;

	for (si = func_specifier.begin();
		 si != func_specifier.end(); si++) {
		(*si)->setSpecifier(specifiers);
	}

	ret_type = specifiers->type->getType(context);
	argTypes = getArgTypeList(context, arguments);

	ftype = FunctionType::get(ret_type, makeArrayRef(argTypes), has_vargs);

	if (!(function = context.module->getFunction(id.name))) {
		function = Function::Create(ftype,
									(specifiers->is_static ? GlobalValue::InternalLinkage
														   : GlobalValue::ExternalLinkage),
									id.name.c_str(), context.module);
	}


	if (block) {
		if (context.currentBlock()) {
			CGERR_Nesting_Function(context);
			CGERR_setLineNum(context, ((NStatement *)this)->line_number);
			CGERR_showAllMsg(context);
		}

		bblock = BasicBlock::Create(getGlobalContext(), "entrypoint", function, 0);
		context.pushBlock(bblock);
		context.builder->SetInsertPoint(context.currentBlock());

		for (it = arguments.begin(), ai = function->arg_begin();
			 it != arguments.end(); it++, ai++) {
			ai->setName((*it)->id.name.c_str());
			AllocaInst *Alloca = context.builder->CreateAlloca(ai->getType(), nullptr, "");
			context.builder->CreateStore(ai, Alloca);
			context.getTopLocals()[(*it)->id.name] = Alloca;
		}

		block->codeGen(context);
		context.builder->CreateRet(context.getCurrentReturnValue());
		context.popAllBlock();
	}

	return function;
}
