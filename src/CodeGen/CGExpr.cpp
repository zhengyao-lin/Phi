#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

Value *
codeGenLoadValue(CodeGenContext& context, Value *V)
{
	if (context.isLValue()
		|| isArrayPointer(V)) {
		return V;
	}

	return context.builder->CreateLoad(V, "");
}

Value *
NIdentifier::codeGen(CodeGenContext& context)
{
	Function *func;

	if (context.getTopLocals().find(name) == context.getTopLocals().end()) {
		if (context.getGlobals().find(name) == context.getGlobals().end()) {
			if (func = context.module->getFunction(name)) {
				return func;
			} else {
				CGERR_Undeclared_Identifier(context, name.c_str());
				CGERR_setLineNum(context, this->line_number);
				CGERR_showAllMsg(context);
				return NULL;
			}
		} else {
			return codeGenLoadValue(context, context.getGlobals()[name]);
		}
	}

	return codeGenLoadValue(context, context.getTopLocals()[name]);
}

Value *
NMethodCall::codeGen(CodeGenContext& context)
{
	int i;
	Value *func_value = func_expr.codeGen(context);
	Value *arg_tmp;
	Function *proto;
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	Type::subtype_iterator arg_it;
	FunctionType *ftype;
	CallInst *call;
	Type *argType;

	if (context.isLValue()) {
		CGERR_Function_Call_As_LValue(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
		CGERR_showAllMsg(context);
	}

	if (func_value->getType()->isPointerTy()
		&& func_value->getType()->getPointerElementType()->isFunctionTy()) {
		ftype = (FunctionType *)func_value->getType()->getPointerElementType();
	} else {
		CGERR_Calling_Non_Function_Value(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
		CGERR_showAllMsg(context);
	}
	proto = Function::Create(ftype,
							 GlobalValue::InternalLinkage);

	for (it = arguments.begin(), arg_it = ftype->param_begin();
		 it != arguments.end(); it++, arg_it++) {
		argType = (arg_it < ftype->param_end() ? ftype->getParamType(arg_it - ftype->param_begin())
												: NULL);
		if (argType && argType->isIntegerTy()) {
			context.currentBitWidth = dyn_cast<IntegerType>(argType)->getIntegerBitWidth();
		}

		arg_tmp = (**it).codeGen(context);
		context.currentBitWidth = 0;

		args.push_back(NAssignmentExpr::doAssignCast(context, arg_tmp,
													 argType, nullptr));
	}

	call = CallInst::Create(func_value, makeArrayRef(args), "", context.currentBlock());
	delete proto;

	return call;
}

static void
doBinaryCast(CodeGenContext& context, Value* &lhs, Value* &rhs)
{
	Type *ltype = lhs->getType();
	Type *rtype = rhs->getType();

	if (isSameType(ltype, rtype)
		|| (ltype->isIntegerTy() && rtype->isIntegerTy())) {
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
		lhs = new PtrToIntInst(lhs, rtype, "",
							   context.currentBlock());
	} else if (ltype->isIntegerTy() && rtype->isPointerTy()) {
		rhs = new PtrToIntInst(rhs, ltype, "",
							   context.currentBlock());
	}

	return;
}

static Value *
tryBinaryMerge(CodeGenContext& context, Value *V1, Instruction::BinaryOps instr, Value *V2)
{
	if (isConstant(V1) && isConstant(V2)) {
		return ConstantExpr::get((unsigned)instr, (Constant *)V1, (Constant *)V2);
	}

	return BinaryOperator::Create(instr, V1, V2, "",
								   context.currentBlock());
}


Value *
NBinaryExpr::codeGen(CodeGenContext& context)
{
	Instruction::BinaryOps instr;
	Value *lop;
	Value *rop;

	lop = lval.codeGen(context);
	rop = rval.codeGen(context);

	doBinaryCast(context, lop, rop);

	if (lop->getType()->isFloatingPointTy() && rop->getType()->isFloatingPointTy()) {
		switch (op) {
			case TADD: 		return context.builder->CreateFAdd(lop, rop, "");
			case TSUB:	 	return context.builder->CreateFSub(lop, rop, "");
			case TMUL: 		return context.builder->CreateFMul(lop, rop, "");
			case TDIV: 		return context.builder->CreateFDiv(lop, rop, "");
			case TMOD: 		return context.builder->CreateFRem(lop, rop, "");
			case TSHL:
			case TSHR:
				CGERR_FP_Value_With_Shift_Operation(context);
				CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
				CGERR_showAllMsg(context);
				break;
		}
	} else if (lop->getType()->isIntegerTy() && rop->getType()->isIntegerTy()) {
		switch (op) {
			case TADD: 		return context.builder->CreateAdd(lop, rop, "");
			case TSUB: 		return context.builder->CreateSub(lop, rop, "");
			case TMUL: 		return context.builder->CreateMul(lop, rop, "");
			case TDIV: 		return context.builder->CreateSDiv(lop, rop, "");
			case TMOD: 		return context.builder->CreateSRem(lop, rop, "");
			case TAND:		return context.builder->CreateAnd(lop, rop, "");
			case TSHL:		return context.builder->CreateShl(lop, rop, "");
			case TSHR:
				if (isBooleanTy(lop->getType())) {
					return context.builder->CreateLShr(lop, rop, "");
				}
				return context.builder->CreateAShr(lop, rop, "");
		}
	}

	CGERR_Unknown_Binary_Operation(context);
	CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
	CGERR_showAllMsg(context);
	return NULL;
}

Value *
NPrefixExpr::codeGen(CodeGenContext& context)
{
	Instruction::BinaryOps instr;
	Value *V;
	Type *T;

	V = operand.codeGen(context);

	if (op == TMUL) {
		return new LoadInst(V->stripPointerCasts(), "",
							 false, context.currentBlock());
	} else if (op == TAND) {
		if (context.isLValue()) {
			return V;
		} else {
			if (dyn_cast<LoadInst>(V)) {
				return dyn_cast<LoadInst>(V)->getPointerOperand();
			} else {
				CGERR_Get_Non_Resident_Value_Address(context);
				CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
				CGERR_showAllMsg(context);
			}
		}
	} else if (op == -1) {
		return NAssignmentExpr::doAssignCast(context, V,
											  type.getType(context), nullptr);
	} else if (op == TSIZEOF) {
		T = type.getType(context);
		if (!T->isVoidTy()) {
			return ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getSizeOfJIT(T));
		} else {
			CGERR_Get_Sizeof_Void(context);
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
			CGERR_showAllMsg(context);
		}
	} else if (op == TALIGNOF) {
		T = type.getType(context);
		if (!T->isVoidTy()) {
			return ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getAlignOfJIT(T));
		} else {
			CGERR_Get_Alignof_Void(context);
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
			CGERR_showAllMsg(context);
		}
	}

	if (V->getType()->isFloatingPointTy()) {
		switch (op) {
			case TADD:
				return V;
				break;
			case TSUB:
				return context.builder->CreateFSub(Constant::getNullValue(V->getType()),
													V, "");
				break;
		}
	} else if (V->getType()->isIntegerTy()) {
		switch (op) {
			case TADD:
				return V;
				break;
			case TSUB:
				return context.builder->CreateSub(Constant::getNullValue(V->getType()),
												   V, "");
			case TNOT:
				return context.builder->CreateNot(V, "");
				break;
		}
	}

	CGERR_Unknown_Unary_Operation(context);
	CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
	CGERR_showAllMsg(context);
	return NULL;
}

#define NUM_MIN(a, b) (a < b ? a : b)

Value *
NAssignmentExpr::doAssignCast(CodeGenContext& context, Value *value,
							  Type *variable_type, Value *variable)
{
	Type *value_type;

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
				return CastInst::Create(CastInst::getCastOpcode(value, true, variable_type, true),
										 value, variable_type, "", context.currentBlock());
			}
		} else if (value_type->isIntegerTy() && variable_type->isFloatingPointTy()) {
			return IntToFPCast_opt(context, value, variable_type);
		} else if (value_type->isFloatingPointTy() && variable_type->isIntegerTy()) {
			return FPToIntCast_opt(context, value, variable_type);
		} else if (value_type->isIntegerTy() && variable_type->isPointerTy()) {
			return new IntToPtrInst(value, variable_type, "",
									 context.currentBlock());
		} else if (value_type->isPointerTy() && variable_type->isIntegerTy()) {
			return new PtrToIntInst(value, variable_type, "",
									 context.currentBlock());
		} else if (isArrayPointerType(value_type)
					&& isArrayPointerType(variable_type)) {
			Type *VET = value_type->getPointerElementType();
			Type *VAET = variable_type->getPointerElementType();

			if (!variable) {
				std::abort();
			}

			context.builder->CreateMemCpy(variable, value,
										  getSizeOfJIT(VET),
										  getAlignOfJIT(VET), false);

			return NULL;
		} else if (value_type->isPointerTy()
					&& value_type->getPointerElementType()->isArrayTy()
					&& variable_type->isPointerTy()) {
			return context.builder->CreateConstInBoundsGEP2_32(value, 0, 0, "");
		} else {
			return new BitCastInst(value, variable_type, "",
									context.currentBlock());
		}
	} else {
		if (value_type->isFloatTy()) {
			if (isConstantFP(value)) {
				return ConstantFP::get(Type::getDoubleTy(getGlobalContext()),
										getConstantDouble(value));
			} else {
				return new FPExtInst(value, Type::getDoubleTy(getGlobalContext()), "",
									  context.currentBlock());
			}
		}
	}

	return value;
}

Value *
NAssignmentExpr::codeGen(CodeGenContext& context)
{
	Value *left;
	Value *right;
	Type *integer_type;

	context.setLValue();
	left = lval.codeGen(context);
	context.resetLValue();

	if (isIntegerPointer(left)) {
		integer_type = dyn_cast<IntegerType>(left->getType()->getPointerElementType());
		context.currentBitWidth = integer_type->getIntegerBitWidth();
	}
	right = rval.codeGen(context);
	context.currentBitWidth = 0;

	right = NAssignmentExpr::doAssignCast(context, right,
										  (!isArrayPointer(left) ? left->getType()->getPointerElementType()
																 : left->getType()), left);

	if (!right) {
		return right;
	}

	return new StoreInst(right, left,
						  false, context.currentBlock());
}

Value *
NFieldExpr::codeGen(CodeGenContext& context)
{
	Value *ret;
	Value *struct_value;
	Type *struct_type;
	FieldMap map;
	UnionFieldMap union_map;
	bool is_union_flag;

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
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
		CGERR_showAllMsg(context);
	}

	if (!struct_type->getStructName().substr(0, strlen(UNION_PREFIX)).compare(UNION_PREFIX)) { // prefix is "union." ?
		is_union_flag = true;
		union_map = context.unions[struct_type->getStructName()];
	} else {
		map = context.structs[struct_type->getStructName()];
	}

	if (is_union_flag) {
		if (union_map.find(field_name.name) != union_map.end()) {
			ret = new BitCastInst(struct_value, union_map[field_name.name]->getPointerTo(),
								  "", context.currentBlock());
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
			CGERR_showAllMsg(context);
		}
	} else {
		if (map.find(field_name.name) != map.end()) {
			ret = context.builder->CreateStructGEP(struct_value, map[field_name.name], "");
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
			CGERR_showAllMsg(context);
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
		idx = index.codeGen(context);
		context.setLValue();
		array_value = operand.codeGen(context);
		context.resetLValue();
	}

	if (isArrayPointer(array_value)) {
		Value *idxs[] = {
			ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
			idx
		};
		ret = context.builder->CreateInBoundsGEP(array_value, makeArrayRef(idxs), "");
	} else if (isPointerPointer(array_value)) {
		ret = context.builder->CreateInBoundsGEP(context.builder->CreateLoad(array_value, ""),
												 idx, "");
	} else {
		CGERR_Get_Non_Array_Element(context);
		CGERR_setLineNum(context, dyn_cast<NExpression>(this)->line_number);
		CGERR_showAllMsg(context);
	}

	return codeGenLoadValue(context, ret);
}
