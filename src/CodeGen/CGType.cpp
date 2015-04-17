#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((NType*)p)->lineno)
#define getFile(p) (((NType*)p)->file_name)

static Type *typeOf(CodeGenContext &context, const NIdentifier& type)
{
	Type *ret;
	if (ret = context.getType(type.name)) {
		return ret;
	}

	if (context.getType(STRUCT_PREFIX + type.name)) {
		CGERR_Suppose_To_Be_Struct_Type(context, type.name.c_str());
		CGERR_setLineNum(context, type.lineno, type.file_name);
		CGERR_showAllMsg(context);
		return NULL;
	}

	if (context.getType(UNION_PREFIX + type.name)) {
		CGERR_Suppose_To_Be_Union_Type(context, type.name.c_str());
		CGERR_setLineNum(context, type.lineno, type.file_name);
		CGERR_showAllMsg(context);
		return NULL;
	}

	CGERR_Unknown_Type_Name(context, type.name.c_str());
	CGERR_setLineNum(context, type.lineno, type.file_name);
	CGERR_showAllMsg(context);

	return NULL;
}

TypeInfoTable
initializeBasicType(CodeGenContext& context)
{
	TypeInfoTable type_info_table;
	extern std::map<std::string, int> type_def;

	type_info_table[string("bool")] = context.builder->getInt1Ty();
	type_def[string("bool")] = 0;
	type_info_table[string("char")] = context.builder->getInt8Ty();
	type_def[string("char")] = 0;
	type_info_table[string("byte")] = context.builder->getInt8Ty();
	type_def[string("byte")] = 0;
	type_info_table[string("short")] = context.builder->getInt16Ty();
	type_def[string("short")] = 0;
	type_info_table[string("int")] = context.builder->getInt32Ty();
	type_def[string("int")] = 0;
	type_info_table[string("long")] = context.builder->getInt64Ty();
	type_def[string("long")] = 0;

	type_info_table[string("float")] = context.builder->getFloatTy();
	type_def[string("float")] = 0;
	type_info_table[string("double")] = context.builder->getDoubleTy();
	type_def[string("double")] = 0;

	type_info_table[string("void")] = context.builder->getVoidTy();
	type_def[string("void")] = 0;

	return type_info_table;
}

Type *
NType::getType(CodeGenContext& context)
{
	return NULL;
}

Type*
NTypeof::getType(CodeGenContext& context)
{
	IRBuilderBase::InsertPoint backup = context.builder->saveIP();
	bool is_lval = context.isLValue();
	Value *tmp_val;

	context.builder->SetInsertPoint(BasicBlock::Create(getGlobalContext())); // Set temp code container

	context.resetLValue();
	tmp_val = operand.codeGen(context);
	if (is_lval) context.setLValue();

	//delete context.builder->GetInsertBlock();
	context.builder->restoreIP(backup);

	return tmp_val->getType();
}

Type *
NIdentifierType::getType(CodeGenContext& context)
{
	return typeOf(context, type);
}

Type *
NBitFieldType::getType(CodeGenContext& context)
{
	if (bit_length > 128) {
		CGERR_Too_Huge_Integer(context, bit_length);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
	} else if (bit_length == 0) {
		CGERR_Integer_Type_With_Size_Of_Zero(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return NULL;
	}

	return Type::getIntNTy(getGlobalContext(), bit_length);
}

Type *
NDerivedType::getType(CodeGenContext& context)
{
	int i;
	Type *base_type = base.getType(context);

	if (!ptr_dim) {
		return base_type;
	}

	if (base_type->getTypeID() == Type::VoidTyID) {
		base_type = Type::getInt8Ty(getGlobalContext()); // (void *) equals (char *)
	}
	for (i = 0; i < ptr_dim; i++) {
		base_type = base_type->getPointerTo();
	}

	return base_type;
}

Type *
NStructType::getType(CodeGenContext& context)
{
	if (struct_decl) { // struct ID { decls... } VAR_NAME;
		return (Type *)(Value *)struct_decl->codeGen(context);
	}

	/*// struct ID VAR_NAME;
	if (context.types.find(STRUCT_PREFIX + id->name) != context.types.end()) {
		return context.types[STRUCT_PREFIX + id->name];
	}

	CGERR_Unknown_Struct_Type(context, id->name.c_str());
	CGERR_setLineNum(context, this->lineno);
	CGERR_showAllMsg(context);*/

	return NULL;
}

Type *
NUnionType::getType(CodeGenContext& context)
{
	if (union_decl) { // union ID { decls... } VAR_NAME;
		return (Type *)(Value *)union_decl->codeGen(context);
	}

	return NULL;
}
