#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

Value *
codeGenLoadValue(CodeGenContext& context, Value *val)
{
	/*if (isArrayPointer(val)) {
		Value *idxs[] = {
			ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
			ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
		};
		return context.builder->CreateInBoundsGEP(val, makeArrayRef(idxs), "");
	}*/

	if (context.isLValue()
		|| isArrayPointer(val)) {
		return val;
	}

	return context.builder->CreateLoad(val, "");
}

Value *
NIdentifier::codeGen(CodeGenContext& context)
{
	Function *func;

	if (context.getGlobals().find(context.formatName(name)) != context.getGlobals().end()) {
		return codeGenLoadValue(context, context.getGlobals()[context.formatName(name)]);
	}

	if (context.getGlobals().find(name) != context.getGlobals().end()) {
		return codeGenLoadValue(context, context.getGlobals()[name]);
	}

	if (context.getTopLocals().find(name) != context.getTopLocals().end()) {
		return codeGenLoadValue(context, context.getTopLocals()[name]);
	}

	if ((func = context.module->getFunction(context.formatName(name)))
		|| (func = context.module->getFunction(name))) {
		return func;
	}

	CGERR_Undeclared_Identifier(context, name.c_str());
	CGERR_setLineNum(context, this->lineno);
	CGERR_showAllMsg(context);
	return NULL;
}

Value *
NMethodCall::codeGen(CodeGenContext& context)
{
	int i;
	Value *func_val = func_expr.codeGen(context);
	Value *tmp;
	Function *proto;
	std::vector<Value*> args;
	ExpressionList::const_iterator expr_it;
	Type::subtype_iterator arg_it;
	FunctionType *ftype;
	CallInst *call;
	Type *arg_type;

	if (context.isLValue()) {
		CGERR_Function_Call_As_LValue(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	if (func_val->getType()->isPointerTy()
		&& func_val->getType()->getPointerElementType()->isFunctionTy()) {
		ftype = (FunctionType *)func_val->getType()->getPointerElementType();
	} else {
		CGERR_Calling_Non_Function_Value(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	proto = Function::Create(ftype,
							 GlobalValue::InternalLinkage);

	for (expr_it = arguments.begin(), arg_it = ftype->param_begin();
		 expr_it != arguments.end(); expr_it++, arg_it++) {
		arg_type = (arg_it < ftype->param_end() ? ftype->getParamType(arg_it - ftype->param_begin())
												: NULL);
		if (arg_type && arg_type->isIntegerTy()) {
			context.current_bit_width = dyn_cast<IntegerType>(arg_type)->getIntegerBitWidth();
		}

		tmp = (**expr_it).codeGen(context);
		context.current_bit_width = 0;

		args.push_back(NAssignmentExpr::doAssignCast(context, tmp,
													 arg_type, nullptr,
													 dyn_cast<NExpression>(this)->lineno));
	}

	call = context.builder->CreateCall(func_val, makeArrayRef(args), "");
	delete proto;

	return call;
}

static void
doBinaryCast(CodeGenContext& context, Value* &lhs, Value* &rhs)
{
	Type *ltype = lhs->getType();
	Type *rtype = rhs->getType();

	if (isSameType(ltype, rtype)) {
		return;
	}

	if (ltype->isFloatingPointTy() && rtype->isFloatingPointTy()) {
		if (ltype->getTypeID() > rtype->getTypeID()) {
			rhs = FPCast_opt(context, rhs, ltype);
			return;
		} else {
			lhs = FPCast_opt(context, lhs, rtype);
			return;
		}
	} else if (ltype->isIntegerTy() && rtype->isIntegerTy()) {
		if (ltype->getIntegerBitWidth() > rtype->getIntegerBitWidth()) {
			rhs = IntCast_opt(context, rhs, ltype);
			return;
		} else {
			lhs = IntCast_opt(context, lhs, rtype);
			return;
		}
	} else if (ltype->isIntegerTy() && rtype->isFloatingPointTy()) {
		lhs = IntToFPCast_opt(context, lhs, rtype);
		return;
	} else if (rtype->isIntegerTy() && ltype->isFloatingPointTy()) {
		rhs = IntToFPCast_opt(context, rhs, ltype);
		return;
	} else if (rtype->isIntegerTy() && ltype->isPointerTy()) {
		lhs = context.builder->CreatePtrToInt(lhs, rtype, "");
	} else if (ltype->isIntegerTy() && rtype->isPointerTy()) {
		rhs = context.builder->CreatePtrToInt(rhs, ltype, "");
	}

	return;
}

Value *
NBinaryExpr::codeGen(CodeGenContext& context)
{
	Value *lhs;
	Value *rhs;

	lhs = lval.codeGen(context);
	rhs = rval.codeGen(context);

	if (lhs->getType()->isPointerTy() && rhs->getType()->isIntegerTy()) {
		rhs = NAssignmentExpr::doAssignCast(context, rhs, Type::getInt64Ty(getGlobalContext()), NULL,
											dyn_cast<NExpression>(this)->lineno);
		if (op == TSUB) {
			rhs = context.builder->CreateSub(ConstantInt::get(rhs->getType(), 0),
											 rhs, "");
		}

		if (isArrayPointer(lhs)) {
			Value *idxs[] = {
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
			};
			lhs = context.builder->CreateInBoundsGEP(lhs, makeArrayRef(idxs), "");
		} else if (context.isLValue()) {
			lhs = context.builder->CreateLoad(lhs);
		}

		return context.builder->CreateInBoundsGEP(lhs, rhs, "");
	}

	doBinaryCast(context, lhs, rhs);

	switch (op) {
		case TLOR:
			return context.builder->CreateOr(context.builder->CreateIsNotNull(lhs, ""),
											  context.builder->CreateIsNotNull(rhs, ""), "");
		case TLAND:
			return context.builder->CreateAnd(context.builder->CreateIsNotNull(lhs, ""),
											   context.builder->CreateIsNotNull(rhs, ""), "");
		case TOR:
			return context.builder->CreateOr(lhs, rhs, "");	
		case TXOR:
			return context.builder->CreateXor(lhs, rhs, "");	
		case TAND:
			return context.builder->CreateAnd(lhs, rhs, "");
	}

	if (lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy()) {
		switch (op) {
			case TADD: 		return context.builder->CreateFAdd(lhs, rhs, "");
			case TSUB:	 	return context.builder->CreateFSub(lhs, rhs, "");
			case TMUL: 		return context.builder->CreateFMul(lhs, rhs, "");
			case TDIV: 		return context.builder->CreateFDiv(lhs, rhs, "");
			case TMOD: 		return context.builder->CreateFRem(lhs, rhs, "");
			case TSHL:
			case TSHR:
				CGERR_FP_Value_With_Shift_Operation(context);
				CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
				CGERR_showAllMsg(context);
				return NULL;
			case TCEQ:		return context.builder->CreateFCmpOEQ(lhs, rhs, "");
			case TCNE:		return context.builder->CreateFCmpONE(lhs, rhs, "");
			case TCLT:		return context.builder->CreateFCmpOLT(lhs, rhs, "");
			case TCGT:		return context.builder->CreateFCmpOGT(lhs, rhs, "");
			case TCLE:		return context.builder->CreateFCmpOLE(lhs, rhs, "");
			case TCGE:		return context.builder->CreateFCmpOGE(lhs, rhs, "");
		}
	} else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
		switch (op) {
			case TADD: 		return context.builder->CreateAdd(lhs, rhs, "");
			case TSUB: 		return context.builder->CreateSub(lhs, rhs, "");
			case TMUL: 		return context.builder->CreateMul(lhs, rhs, "");
			case TDIV: 		return context.builder->CreateSDiv(lhs, rhs, "");
			case TMOD: 		return context.builder->CreateSRem(lhs, rhs, "");
			case TSHL:		return context.builder->CreateShl(lhs, rhs, "");
			case TSHR:
				return context.builder->CreateAShr(lhs, rhs, "");
			case TCEQ:		return context.builder->CreateICmpEQ(lhs, rhs, "");
			case TCNE:		return context.builder->CreateICmpNE(lhs, rhs, "");
			case TCLT:		return context.builder->CreateICmpSLT(lhs, rhs, "");
			case TCGT:		return context.builder->CreateICmpSGT(lhs, rhs, "");
			case TCLE:		return context.builder->CreateICmpSLE(lhs, rhs, "");
			case TCGE:		return context.builder->CreateICmpSGE(lhs, rhs, "");
		}
	}

	CGERR_Unknown_Binary_Operation(context);
	CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
	CGERR_showAllMsg(context);
	return NULL;
}

inline Value *
getLoadOperand(CodeGenContext& context, Value *val, bool if_delete)
{
	LoadInst *load_inst;
	GetElementPtrInst *get_ptr_inst;

	if (load_inst = dyn_cast<LoadInst>(val)) {
		val = load_inst->getPointerOperand();

		if (if_delete) {
			load_inst->removeFromParent();
			delete load_inst;
		}

		return val;
	} else if (get_ptr_inst = dyn_cast<GetElementPtrInst>(val)) {
		val = get_ptr_inst->getPointerOperand();

		if (if_delete) {
			get_ptr_inst->removeFromParent();
			delete get_ptr_inst;
		}

		return val;
	}

	return NULL;
}

Value *
NPrefixExpr::codeGen(CodeGenContext& context)
{
	LoadInst *load_inst;
	GetElementPtrInst *get_ptr_inst;
	Value *val_tmp;
	Value *val_ptr;
	Value *rhs;
	Type *val_type;
	Type *type_expr_operand;

	val_tmp = operand.codeGen(context);
	type_expr_operand = type.getType(context);

	if (op == TMUL) {
		if (isArrayPointer(val_tmp)) {
			Value *idxs[] = {
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
			};
			return context.builder->CreateInBoundsGEP(val_tmp,
													   makeArrayRef(idxs), "");
		}
		if (context.isLValue()
			&& (typeid(operand) == typeid(NBinaryExpr) // *( expr [+-] expr ) = ?
			|| typeid(operand) == typeid(NPrefixExpr) // *( ++expr) = ?
			|| typeid(operand) == typeid(NIncDecExpr))) {
			return val_tmp;
		}

		return context.builder->CreateLoad(val_tmp, "");
	} else if (op == TAND) {
		if (context.isLValue()) {
			return val_tmp;
		} else {
			if (val_tmp = getLoadOperand(context, val_tmp, true)) {
				return val_tmp;
			} else {
				CGERR_Get_Non_Resident_Value_Address(context);
				CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
				CGERR_showAllMsg(context);
				return NULL;
			}
		}
	} else if (op == -1) {
		return NAssignmentExpr::doAssignCast(context, val_tmp,
											  type_expr_operand, nullptr,
											  dyn_cast<NExpression>(this)->lineno);
	} else if (op == TSIZEOF) {
		if (!type_expr_operand->isVoidTy()) {
			return ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getSizeOfJIT(type_expr_operand));
		} else {
			CGERR_Get_Sizeof_Void(context);
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
			CGERR_showAllMsg(context);
			return NULL;
		}
	} else if (op == TALIGNOF) {
		if (!type_expr_operand->isVoidTy()) {
			return ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getAlignOfJIT(type_expr_operand));
		} else {
			CGERR_Get_Alignof_Void(context);
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
			CGERR_showAllMsg(context);
			return NULL;
		}
	}

	val_type = val_tmp->getType();

	if (op == TINC || op == TDEC) {
		if (val_type->isPointerTy()
			&& !isArrayPointer(val_tmp)) {
			Value *ret_tmp;
			rhs = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);

			if (op == TDEC) {
				rhs = context.builder->CreateSub(ConstantInt::get(rhs->getType(), 0),
												 rhs, "");
			}

			if (context.isLValue()) {
				val_tmp = context.builder->CreateLoad(val_tmp);
			}

			ret_tmp = context.builder->CreateInBoundsGEP(val_tmp, rhs, "");
			context.builder->CreateStore(ret_tmp,
										 getLoadOperand(context, val_tmp, false));

			return ret_tmp;
		}
		if (!(val_ptr = getLoadOperand(context, val_tmp, false))) {
			CGERR_Inc_Dec_Unassignable_Value(context);
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
			CGERR_showAllMsg(context);
			return NULL;
		}
	}

	switch (op) {
		case TLNOT:
			return context.builder->CreateNot(val_tmp, "");
			break;
	}

	if (val_type->isFloatingPointTy()) {
		switch (op) {
			case TINC: {
				Value *add_inst;
				add_inst = context.builder->CreateFAdd(val_tmp,
													   ConstantFP::get(val_type, 1.0));
				context.builder->CreateStore(add_inst,
											 val_ptr);
				return add_inst;
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateFSub(val_tmp,
													   ConstantFP::get(val_type, 1.0));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				return sub_inst;
				break;
			}
			case TADD:
				return val_tmp;
				break;
			case TSUB:
				return context.builder->CreateFSub(Constant::getNullValue(val_type),
													val_tmp, "");
				break;
		}
	} else if (val_type->isIntegerTy()) {
		switch (op) {
			case TINC: {
				Value *add_inst;
				add_inst = context.builder->CreateAdd(val_tmp,
													  ConstantInt::get(val_type, 1));
				context.builder->CreateStore(add_inst,
											 val_ptr);
				return add_inst;
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateSub(val_tmp,
													  ConstantInt::get(val_type, 1));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				return sub_inst;
				break;
			}
			case TADD:
				return val_tmp;
				break;
			case TSUB:
				return context.builder->CreateSub(Constant::getNullValue(val_type),
												   val_tmp, "");
			case TNOT:
				return context.builder->CreateNot(val_tmp, "");
				break;
		}
	}

	CGERR_Unknown_Unary_Operation(context);
	CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
	CGERR_showAllMsg(context);
	return NULL;
}

#define NUM_MIN(a, b) (a < b ? a : b)

template <typename T>
T getConstantArrayElementCastTo(ConstantDataSequential *const_data_seq, int i)
{
	if (const_data_seq->getElementType()->isIntegerTy()) {
		return const_data_seq->getElementAsInteger(i);
	} else if (const_data_seq->getElementType()->isFloatingPointTy()) {
		return const_data_seq->getElementAsDouble(i);
	}

	return 0;
}

static Constant *
doAlignArray(CodeGenContext& context, Constant *array, Type *dest_type,
			 uint64_t size, int lineno)
{
	unsigned i;
	ConstantDataSequential *const_data_seq = dyn_cast<ConstantDataSequential>(array);

	switch (dest_type->getTypeID()) {
		case Type::IntegerTyID:
			switch (getBitWidth(dest_type)) {
				case 8: {
					vector<uint8_t> const_arr;
					for (i = 0; i < size; i++) {
						const_arr.push_back(getConstantArrayElementCastTo<int>(const_data_seq, i));
					}
					return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
				}
				case 16: {
					vector<uint16_t> const_arr;
					for (i = 0; i < size; i++) {
						const_arr.push_back(getConstantArrayElementCastTo<int>(const_data_seq, i));
					}
					return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
				}
				case 32: {
					vector<uint32_t> const_arr;
					for (i = 0; i < size; i++) {
						const_arr.push_back(getConstantArrayElementCastTo<int>(const_data_seq, i));
					}
					return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
				}
				case 64: {
					vector<uint64_t> const_arr;
					for (i = 0; i < size; i++) {
						const_arr.push_back(getConstantArrayElementCastTo<int>(const_data_seq, i));
					}
					return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
				}
				default:
					CGERR_Unsupport_Integer_Bitwidth_For_Data_Array(context);
					CGERR_setLineNum(context, lineno);
					CGERR_showAllMsg(context);
					return NULL;
			}
			break;
		case Type::FloatTyID: {
			vector<float> const_arr;
			for (i = 0; i < size; i++) {
				const_arr.push_back(getConstantArrayElementCastTo<float>(const_data_seq, i));
			}
			return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
		}
		case Type::DoubleTyID: {
			vector<double> const_arr;
			for (i = 0; i < size; i++) {
				const_arr.push_back(getConstantArrayElementCastTo<double>(const_data_seq, i));
			}
			return ConstantDataArray::get(getGlobalContext(), makeArrayRef(const_arr));
		}
		default:
			std::abort();
			break;
	}

	return NULL;
}

Value *
NAssignmentExpr::doAssignCast(CodeGenContext& context, Value *value,
							  Type *variable_type, Value *variable, int lineno = -1)
{
	Type *value_type;
	Value *val_tmp;

	value_type = value->getType();

	if (variable_type && isSameType(value_type, variable_type)) {
		return value;
	}

	if (variable_type != NULL) {
		if ((value_type->isFloatingPointTy() && variable_type->isFloatingPointTy())
			|| (value_type->isIntegerTy() && variable_type->isIntegerTy())) {
			if (isConstantInt(value)) {
				return ConstantInt::get(variable_type, getConstantInt(value), true);
			} else if (isConstantFP(value)) {
				return ConstantFP::get(variable_type, getConstantDouble(value));
			} else {
				return context.builder->CreateCast(CastInst::getCastOpcode(value, true, variable_type, true),
													value, variable_type, "");
			}
		} else if (value_type->isIntegerTy() && variable_type->isFloatingPointTy()) {
			return IntToFPCast_opt(context, value, variable_type);
		} else if (value_type->isFloatingPointTy() && variable_type->isIntegerTy()) {
			return FPToIntCast_opt(context, value, variable_type);
		} else if (value_type->isIntegerTy() && variable_type->isPointerTy()) {
			return context.builder->CreateIntToPtr(value, variable_type, "");
		} else if (value_type->isPointerTy() && variable_type->isIntegerTy()) {
			return context.builder->CreatePtrToInt(value, variable_type, "");
		} else if (isArrayType(value_type)
					&& isArrayType(variable_type)
					&& !context.currentBlock()) {
			ArrayType *val_elem_type = dyn_cast<ArrayType>(value_type);
			ArrayType *var_elem_type = dyn_cast<ArrayType>(variable_type);
			if (val_elem_type->getNumElements()
				!= var_elem_type->getNumElements()) {
				value = doAlignArray(context, dyn_cast<Constant>(value),
									 var_elem_type->getArrayElementType(),
									 var_elem_type->getNumElements(), lineno);
			}

			return value;
		} else if (isArrayType(value_type)
					&& isPointerType(variable_type)
					&& !context.currentBlock()) {
			return value;
		} else if (isArrayPointerType(value_type)
					&& isArrayPointerType(variable_type)) {
			Type *val_elem_type = value_type->getPointerElementType();
			if (!variable) {
				std::abort();
			}
			context.builder->CreateMemCpy(variable, value,
										  getSizeOfJIT(val_elem_type),
										  getAlignOfJIT(val_elem_type), false);
			return NULL;
		} else if (isArrayPointerType(value_type) && isPointerType(variable_type)) {
			val_tmp = dyn_cast<Value>(context.builder->CreateConstInBoundsGEP2_32(value, 0, 0, ""));
			if (isSameType(val_tmp->getType(), variable_type)) {
				return val_tmp;
			}
			return context.builder->CreateBitCast(val_tmp, variable_type, "");
		} else {
			return context.builder->CreateBitCast(value, variable_type, "");
		}
	} else {
		if (value_type->isFloatTy()) {
			if (isConstantFP(value)) {
				return ConstantFP::get(Type::getDoubleTy(getGlobalContext()),
										getConstantDouble(value));
			} else {
				return context.builder->CreateFPExt(value, Type::getDoubleTy(getGlobalContext()), "");
			}
		}
	}

	return value;
}

Value *
NAssignmentExpr::codeGen(CodeGenContext& context)
{
	Value *lhs;
	Value *rhs;
	Type *integer_type;

	context.setLValue();
	lhs = lval.codeGen(context);
	context.resetLValue();

	if (!isPointer(lhs)
		/*|| typeid(lval) == typeid(NBinaryExpr)*/) {
		CGERR_Unassignable_LValue(context);
		CGERR_setLineNum(context, ((NExpression*)this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	if (isIntegerPointer(lhs)) {
		integer_type = dyn_cast<IntegerType>(lhs->getType()->getPointerElementType());
		context.current_bit_width = integer_type->getIntegerBitWidth();
	}
	rhs = rval.codeGen(context);
	context.current_bit_width = 0;

	rhs = NAssignmentExpr::doAssignCast(context, rhs,
										(!isArrayPointer(lhs) ? lhs->getType()->getPointerElementType()
															  : lhs->getType()), lhs,
										((NExpression*)this)->lineno);

	if (!rhs) {
		return rhs;
	}

	context.builder->CreateStore(rhs, lhs, false);

	return rhs;
}

Value *
NFieldExpr::codeGen(CodeGenContext& context)
{
	Value *ret;
	Value *struct_value;
	Type *struct_type;
	FieldMap map;
	UnionFieldMap union_map;
	bool is_union_flag = false;

	if (context.isLValue()) {
		struct_value = operand.codeGen(context);
	} else {
		context.setLValue();
		struct_value = operand.codeGen(context);
		context.resetLValue();
	}

	if (isStructPointer(struct_value)) {
		struct_type = struct_value->getType()->getPointerElementType();
	} else {
		CGERR_Get_Non_Structure_Type_Field(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	if (!struct_type->getStructName().substr(0, strlen(UNION_PREFIX)).compare(UNION_PREFIX)) { // prefix is "union." ?
		is_union_flag = true;
		if (context.getUnion(struct_type->getStructName())) {
			union_map = *context.getUnion(struct_type->getStructName());
		} else {
			abort();
		}
	} else {
		if (context.getStruct(struct_type->getStructName())) {
			map = *context.getStruct(struct_type->getStructName());
		} else {
			abort();
		}
	}

	if (is_union_flag) {
		if (union_map.find(field_name.name) != union_map.end()) {
			ret = context.builder->CreateBitCast(struct_value, union_map[field_name.name]->getPointerTo(),
												 "");
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
			CGERR_showAllMsg(context);
			return NULL;
		}
	} else {
		if (map.find(field_name.name) != map.end()) {
			ret = context.builder->CreateStructGEP(struct_value, map[field_name.name], "");
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
			CGERR_showAllMsg(context);
			return NULL;
		}
	}

	return codeGenLoadValue(context, ret);
}

Value *
NArrayExpr::codeGen(CodeGenContext& context)
{
	Value *ret;
	Value *array_value;
	Value *idx;

	if (context.isLValue()) {
		array_value = operand.codeGen(context);
		context.resetLValue();
		idx = index.codeGen(context);
		context.setLValue();
	} else {
		context.setLValue();
		array_value = operand.codeGen(context);
		context.resetLValue();
		idx = index.codeGen(context);
	}

	idx = NAssignmentExpr::doAssignCast(context, idx, Type::getInt64Ty(getGlobalContext()), NULL,
										dyn_cast<NExpression>(this)->lineno);

	if (isArrayPointer(array_value)) {
		if (typeid(operand) == typeid(NBinaryExpr)) {
			ret = context.builder->CreateInBoundsGEP(array_value, idx, "");
		} else {
			Value *idxs[] = {
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
				idx
			};
			ret = context.builder->CreateInBoundsGEP(array_value, makeArrayRef(idxs), "");
		}
	} else if (isPointer(array_value)) {
		if (typeid(operand) != typeid(NBinaryExpr)) {
			array_value = context.builder->CreateLoad(array_value);
		}
		ret = context.builder->CreateInBoundsGEP(array_value,
												 idx, "");
	} else {
		CGERR_Get_Non_Array_Element(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	return codeGenLoadValue(context, ret);
}

Value *
NIncDecExpr::codeGen(CodeGenContext& context)
{
	Type *val_type;
	Value *val_tmp;
	Value *val_ptr;
	Value *rhs;

	val_tmp = operand.codeGen(context);
	val_type = val_tmp->getType();

	if (val_type->isPointerTy()
		&& !isArrayPointer(val_tmp)) {
		rhs = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);

		if (op == TDEC) {
			rhs = context.builder->CreateSub(ConstantInt::get(rhs->getType(), 0),
											 rhs, "");
		}

		if (context.isLValue()) {
			val_tmp = context.builder->CreateLoad(val_tmp);
			context.builder->CreateStore(context.builder->CreateInBoundsGEP(val_tmp, rhs, ""),
										 getLoadOperand(context, val_tmp, false));
		} else {
			context.builder->CreateStore(context.builder->CreateInBoundsGEP(val_tmp, rhs, ""),
										 getLoadOperand(context, val_tmp, false));
		}

		return val_tmp;
	}

	if (!(val_ptr = getLoadOperand(context, val_tmp, false))) {
		CGERR_Inc_Dec_Unassignable_Value(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->lineno);
		CGERR_showAllMsg(context);
		return NULL;
	}

	if (val_type->isFloatingPointTy()) {
		switch (op) {
			case TINC: {
				Value *add_inst;
				add_inst = context.builder->CreateFAdd(val_tmp,
													   ConstantFP::get(val_type, 1.0));
				context.builder->CreateStore(add_inst,
											 val_ptr);
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateFSub(val_tmp,
													   ConstantFP::get(val_type, 1.0));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				break;
			}
		}
	} else if (val_type->isIntegerTy()) {
		switch (op) {
			case TINC: {
				Value *add_inst;
				add_inst = context.builder->CreateAdd(val_tmp,
													  ConstantInt::get(val_type, 1));
				context.builder->CreateStore(add_inst,
											 val_ptr);
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateSub(val_tmp,
													  ConstantInt::get(val_type, 1));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				break;
			}
		}
	}

	return val_tmp;
}
