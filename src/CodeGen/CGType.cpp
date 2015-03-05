#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

static Type *typeOf(CodeGenContext &context, const NIdentifier& type)
{
	if (context.types.find(type.name) != context.types.end()) {
		return context.types[type.name];
	}

	if (context.types.find(STRUCT_PREFIX + type.name) != context.types.end()) {
		CGERR_Suppose_To_Be_Struct_Type(context, type.name.c_str());
		CGERR_setLineNum(context, type.line_number);
		CGERR_showAllMsg(context);
		return NULL;
	}

	CGERR_Unknown_Type_Name(context, type.name.c_str());
	CGERR_setLineNum(context, type.line_number);
	CGERR_showAllMsg(context);

	return NULL;
}

TypeInfoTable
initializeBasicType(CodeGenContext& context)
{
	TypeInfoTable type_info_table;

	type_info_table[string("bit")] = context.builder->getInt1Ty();
	type_info_table[string("bool")] = context.builder->getInt1Ty();
	type_info_table[string("char")] = context.builder->getInt8Ty();
	type_info_table[string("byte")] = context.builder->getInt8Ty();
	type_info_table[string("short")] = context.builder->getInt16Ty();
	type_info_table[string("int")] = context.builder->getInt32Ty();
	type_info_table[string("long")] = context.builder->getInt64Ty();

	type_info_table[string("Int8")] = context.builder->getInt8Ty();
	type_info_table[string("Int16")] = context.builder->getInt16Ty();
	type_info_table[string("Int32")] = context.builder->getInt32Ty();
	type_info_table[string("Int64")] = context.builder->getInt64Ty();

	type_info_table[string("float")] = context.builder->getFloatTy();
	type_info_table[string("single")] = context.builder->getFloatTy();
	type_info_table[string("double")] = context.builder->getDoubleTy();

	type_info_table[string("string")] = context.builder->getInt8PtrTy();
	type_info_table[string("void")] = context.builder->getVoidTy();

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
	bool isLV_backup = context.isLValue();
	Value *V;

	context.builder->SetInsertPoint(BasicBlock::Create(getGlobalContext())); // Set temp code container

	context.resetLValue();
	V = operand.codeGen(context);
	if (isLV_backup) context.setLValue();

	delete context.builder->GetInsertBlock();
	context.builder->restoreIP(backup);

	if (isArrayPointer(V)) {
		return V->getType()->getArrayElementType();
	}

	return V->getType();
}

Type *
NIdentifierType::getType(CodeGenContext& context)
{
	return typeOf(context, type);
}

Type *
NBitFieldType::getType(CodeGenContext& context)
{
	if (N > 128) {
		CGERR_Too_Huge_Integer(context, N);
		CGERR_setLineNum(context, ((NType*)this)->line_number);
		CGERR_showAllMsg(context);
	} else if (N == 0) {
		CGERR_Integer_Type_With_Size_Of_Zero(context);
		CGERR_setLineNum(context, ((NType*)this)->line_number);
		CGERR_showAllMsg(context);
	}

	return Type::getIntNTy(getGlobalContext(), N);
}

Type *
NDerivedType::getType(CodeGenContext& context)
{
	int i;
	Type *base_type = base.getType(context);

	if (ptrDim == 0 && arrDim == 0) {
		return base_type;
	}

	if (base_type->getTypeID() == Type::VoidTyID) {
		base_type = Type::getInt8Ty(getGlobalContext()); // (void *) equals (char *)
	}
	for (i = 0; i < ptrDim; i++) {
		base_type = base_type->getPointerTo();
	}

	return base_type;
}

Type *
NStructType::getType(CodeGenContext& context)
{
	if (struct_decl) { // struct ID { decls... } VAR_NAME;
		return (Type *)struct_decl->codeGen(context);
	}

	// struct ID VAR_NAME;
	if (context.types.find(STRUCT_PREFIX + id->name) != context.types.end()) {
		return context.types[STRUCT_PREFIX + id->name];
	}

	CGERR_Unknown_Struct_Type(context, id->name.c_str());
	CGERR_setLineNum(context, this->line_number);
	CGERR_showAllMsg(context);
	return nullptr;
}

Type *
NUnionType::getType(CodeGenContext& context)
{
	if (union_decl) { // union ID { decls... } VAR_NAME;
		return (Type *)union_decl->codeGen(context);
	}

	// union ID VAR_NAME;
	if (context.types.find(UNION_PREFIX + id->name) != context.types.end()) {
		return context.types[UNION_PREFIX + id->name];
	}

	CGERR_Unknown_Union_Type(context, id->name.c_str());
	CGERR_setLineNum(context, this->line_number);
	CGERR_showAllMsg(context);
	return nullptr;
}
