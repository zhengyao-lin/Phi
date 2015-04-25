#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define getLine(p) (((NExpression *)this)->lineno)
#define getFile(p) (((NExpression *)this)->file_name)

CGValue
codeGenLoadValue(CodeGenContext& context, Value *val)
{
	/*if (isArrayPointer(val)) {
		Value *idxs[] = {
			ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
			ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
		};
		return context.builder->CreateInBoundsGEP(val, makeArrayRef(idxs), "");
	}*/

	if (context.isLValue()) {
		return CGValue(val);
	}

	return CGValue(context.builder->CreateLoad(val, ""));
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

CGValue
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
		return CGValue(func);
	}

	CGERR_Undeclared_Identifier(context, name.c_str());
	CGERR_setLineNum(context, getLine(this), getFile(this));
	CGERR_showAllMsg(context);
	return CGValue();
}

CGValue
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
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	if (func_val->getType()->isPointerTy()
		&& func_val->getType()->getPointerElementType()->isFunctionTy()) {
		ftype = (FunctionType *)func_val->getType()->getPointerElementType();
	} else {
		CGERR_Calling_Non_Function_Value(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	proto = Function::Create(ftype,
							 GlobalValue::InternalLinkage);

	for (expr_it = arguments.begin(), arg_it = ftype->param_begin();
		 expr_it != arguments.end() && (arg_it != ftype->param_end() || ftype->isVarArg());
		 expr_it++, (arg_it != ftype->param_end() ? arg_it++ : 0)) {
		arg_type = (arg_it < ftype->param_end() ? ftype->getParamType(arg_it - ftype->param_begin())
												: NULL);

		tmp = (**expr_it).codeGen(context);
		context.current_bit_width = 0;

		args.push_back(NAssignmentExpr::doAssignCast(context, tmp,
													 arg_type, nullptr,
													 getLine(this), getFile(this)));
	}

	if (expr_it != arguments.end() || arg_it != ftype->param_end()) {
		CGERR_Unable_Match_Arguments(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	call = context.builder->CreateCall(func_val, makeArrayRef(args), "");
	delete proto;

	return CGValue(call);
}

static void
doBinaryCast(CodeGenContext& context, Value* &lhs, Value* &rhs)
{
	Type *ltype = lhs->getType();
	Type *rtype = rhs->getType();

	if (ltype->isArrayTy()) {
		lhs = context.builder->CreateConstInBoundsGEP2_32(getLoadOperand(context, lhs, true), 0, 0, "");
		ltype = lhs->getType();
	}
	if (rtype->isArrayTy()) {
		rhs = context.builder->CreateConstInBoundsGEP2_32(getLoadOperand(context, rhs, true), 0, 0, "");
		rtype = rhs->getType();
	}

	if (ltype->isPointerTy() && rtype->isPointerTy()) {
		lhs = context.builder->CreatePtrToInt(lhs, context.builder->getInt64Ty(), "");
		rhs = context.builder->CreatePtrToInt(rhs, context.builder->getInt64Ty(), "");
		return;
	}

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
	} else if (ltype->isPointerTy()) {
		lhs = context.builder->CreatePtrToInt(lhs, rtype, "");
	} else if (rtype->isPointerTy()) {
		rhs = context.builder->CreatePtrToInt(rhs, ltype, "");
	}

	return;
}

#define setBlock(b) (context.pushBlock(b), \
					 (context.currentBlock()->getTerminator() ? \
					 context.builder->SetInsertPoint(context.currentBlock()->getTerminator()) : \
					 context.builder->SetInsertPoint(context.currentBlock())))
#define _rhs_getParent_O1_ (dyn_cast<Instruction>(rhs) ? \
							dyn_cast<Instruction>(rhs)->getParent() :\
							lhs_true) // only use in emitLogicalExpr function

static Value *
emitLogicalExpr(CodeGenContext& context, NExpression &lval, NExpression &rval, bool is_or)
{
	Value *lhs;
	Value *rhs;
	BasicBlock *lhs_true;
	BasicBlock *lhs_end;
	BasicBlock *orig_block;
	PHINode *phi_node;

	lhs = context.builder->CreateIsNotNull(lval.codeGen(context));
	orig_block = context.currentBlock();

	lhs_true = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								  context.current_end_block);
	lhs_end = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								 context.current_end_block);

	if (is_or) {
		context.builder->CreateCondBr(lhs, lhs_end, lhs_true);
	} else {
		context.builder->CreateCondBr(lhs, lhs_true, lhs_end);
	}

	setBlock(lhs_true);

	rhs = context.builder->CreateIsNotNull(rval.codeGen(context));
	context.builder->CreateBr(lhs_end);

	setBlock(lhs_end);
	phi_node = context.builder->CreatePHI(context.builder->getInt1Ty(), 2);

	phi_node->addIncoming(context.builder->getInt1(is_or), orig_block);
	phi_node->addIncoming(rhs, _rhs_getParent_O1_);

	return phi_node;
}

inline bool
pointerAllowedExpr(int op)
{
	switch (op) {
		case TADD:
		case TSUB:
			return true;
	}
	return false;
}

CGValue
NBinaryExpr::codeGen(CodeGenContext& context)
{
	Value *lhs;
	Value *rhs;

	if (op == TLAND) {
		return CGValue(emitLogicalExpr(context, lval, rval, false));
	} else if (op == TLOR) {
		return CGValue(emitLogicalExpr(context, lval, rval, true));
	}

	lhs = lval.codeGen(context);
	rhs = rval.codeGen(context);

	if ((lhs->getType()->isPointerTy() || lhs->getType()->isArrayTy())
		&& rhs->getType()->isIntegerTy()
		&& pointerAllowedExpr(op)) {
		rhs = NAssignmentExpr::doAssignCast(context, rhs, Type::getInt64Ty(getGlobalContext()), NULL,
											getLine(this), getFile(this));
		if (op == TSUB) {
			rhs = context.builder->CreateSub(ConstantInt::get(rhs->getType(), 0),
											 rhs, "");
		}

		if (!context.isLValue() && isArray(lhs)) {
			lhs = getLoadOperand(context, lhs, true);
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

		return CGValue(context.builder->CreateInBoundsGEP(lhs, rhs, ""));
	}

	doBinaryCast(context, lhs, rhs);

	switch (op) {
		case TOR:
			return CGValue(context.builder->CreateOr(lhs, rhs, ""));	
		case TXOR:
			return CGValue(context.builder->CreateXor(lhs, rhs, ""));	
		case TAND:
			return CGValue(context.builder->CreateAnd(lhs, rhs, ""));
	}

	if (lhs->getType()->isFloatingPointTy() && rhs->getType()->isFloatingPointTy()) {
		switch (op) {
			case TADD: 		return CGValue(context.builder->CreateFAdd(lhs, rhs, ""));
			case TSUB:	 	return CGValue(context.builder->CreateFSub(lhs, rhs, ""));
			case TMUL: 		return CGValue(context.builder->CreateFMul(lhs, rhs, ""));
			case TDIV: 		return CGValue(context.builder->CreateFDiv(lhs, rhs, ""));
			case TMOD: 		return CGValue(context.builder->CreateFRem(lhs, rhs, ""));
			case TSHL:
			case TSHR:
				CGERR_FP_Value_With_Shift_Operation(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
				return CGValue();
			case TCEQ:		return CGValue(context.builder->CreateFCmpOEQ(lhs, rhs, ""));
			case TCNE:		return CGValue(context.builder->CreateFCmpONE(lhs, rhs, ""));
			case TCLT:		return CGValue(context.builder->CreateFCmpOLT(lhs, rhs, ""));
			case TCGT:		return CGValue(context.builder->CreateFCmpOGT(lhs, rhs, ""));
			case TCLE:		return CGValue(context.builder->CreateFCmpOLE(lhs, rhs, ""));
			case TCGE:		return CGValue(context.builder->CreateFCmpOGE(lhs, rhs, ""));
		}
	} else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
		switch (op) {
			case TADD: 		return CGValue(context.builder->CreateAdd(lhs, rhs, ""));
			case TSUB: 		return CGValue(context.builder->CreateSub(lhs, rhs, ""));
			case TMUL: 		return CGValue(context.builder->CreateMul(lhs, rhs, ""));
			case TDIV: 		return CGValue(context.builder->CreateSDiv(lhs, rhs, ""));
			case TMOD: 		return CGValue(context.builder->CreateSRem(lhs, rhs, ""));
			case TSHL:		return CGValue(context.builder->CreateShl(lhs, rhs, ""));
			case TSHR:
				return CGValue(context.builder->CreateAShr(lhs, rhs, ""));
			case TCEQ:		return CGValue(context.builder->CreateICmpEQ(lhs, rhs, ""));
			case TCNE:		return CGValue(context.builder->CreateICmpNE(lhs, rhs, ""));
			case TCLT:		return CGValue(context.builder->CreateICmpSLT(lhs, rhs, ""));
			case TCGT:		return CGValue(context.builder->CreateICmpSGT(lhs, rhs, ""));
			case TCLE:		return CGValue(context.builder->CreateICmpSLE(lhs, rhs, ""));
			case TCGE:		return CGValue(context.builder->CreateICmpSGE(lhs, rhs, ""));
		}
	}

	CGERR_Unknown_Binary_Operation(context);
	CGERR_setLineNum(context, getLine(this), getFile(this));
	CGERR_showAllMsg(context);
	return CGValue();
}

CGValue
NPrefixExpr::codeGen(CodeGenContext& context)
{
	LoadInst *load_inst;
	GetElementPtrInst *get_ptr_inst;
	Value *val_tmp;
	Value *val_ptr;
	Value *rhs;
	Type *val_type;
	Type *type_expr_operand;

	if (op == TDCOLON) {
		string backup = context.current_namespace;
		context.current_namespace = "";
		val_tmp = operand.codeGen(context);
		context.current_namespace = backup;
		return CGValue(val_tmp);
	}

	val_tmp = operand.codeGen(context);
	type_expr_operand = type.getType(context);

	if (op == TMUL) {
		if (isArray(val_tmp)) {
			Value *idxs[] = {
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
			};
			return CGValue(context.builder->CreateInBoundsGEP(getLoadOperand(context, val_tmp, true),
													   makeArrayRef(idxs), ""));
		} else if (isArrayPointer(val_tmp) && context.isLValue()) {
			Value *idxs[] = {
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0),
				ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0)
			};
			return CGValue(context.builder->CreateInBoundsGEP(val_tmp,
													   makeArrayRef(idxs), ""));
		}

		if (context.isLValue()
			&& (typeid(operand) == typeid(NBinaryExpr) // *( expr [+-] expr ) = ?
			|| typeid(operand) == typeid(NPrefixExpr) // *( ++expr) = ?
			|| typeid(operand) == typeid(NIncDecExpr))) {
			return CGValue(val_tmp);
		}

		return CGValue(context.builder->CreateLoad(val_tmp, ""));
	} else if (op == TAND) {
		if (context.isLValue()) {
			return CGValue(val_tmp);
		} else {
			if (val_tmp = getLoadOperand(context, val_tmp, true)) {
				return CGValue(val_tmp);
			} else {
				CGERR_Get_Non_Resident_Value_Address(context);
				CGERR_setLineNum(context, getLine(this), getFile(this));
				CGERR_showAllMsg(context);
				return CGValue();
			}
		}
	} else if (op == -1) {
		return CGValue(NAssignmentExpr::doAssignCast(context, val_tmp,
											  type_expr_operand, nullptr,
											  getLine(this), getFile(this)));
	} else if (op == TSIZEOF) {
		if (!type_expr_operand->isVoidTy()) {
			return CGValue(ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getSizeOfJIT(type_expr_operand)));
		} else {
			CGERR_Get_Sizeof_Void(context);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	} else if (op == TALIGNOF) {
		if (!type_expr_operand->isVoidTy()) {
			return CGValue(ConstantInt::get(Type::getInt64Ty(getGlobalContext()),
									 getAlignOfJIT(type_expr_operand)));
		} else {
			CGERR_Get_Alignof_Void(context);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	}

	val_type = val_tmp->getType();

	if (op == TINC || op == TDEC) {
		if (val_type->isPointerTy()) {
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

			return CGValue(ret_tmp);
		}
		if (!(val_ptr = getLoadOperand(context, val_tmp, false))) {
			CGERR_Inc_Dec_Unassignable_Value(context);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	}

	switch (op) {
		case TLNOT:
			return CGValue(context.builder->CreateIsNull(val_tmp, ""));
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
				return CGValue(add_inst);
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateFSub(val_tmp,
													   ConstantFP::get(val_type, 1.0));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				return CGValue(sub_inst);
				break;
			}
			case TADD:
				return CGValue(val_tmp);
				break;
			case TSUB:
				return CGValue(context.builder->CreateFSub(Constant::getNullValue(val_type),
													val_tmp, ""));
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
				return CGValue(add_inst);
				break;
			}
			case TDEC: {
				Value *sub_inst;
				sub_inst = context.builder->CreateSub(val_tmp,
													  ConstantInt::get(val_type, 1));
				context.builder->CreateStore(sub_inst,
											 val_ptr);
				return CGValue(sub_inst);
				break;
			}
			case TADD:
				return CGValue(val_tmp);
				break;
			case TSUB:
				return CGValue(context.builder->CreateSub(Constant::getNullValue(val_type),
												   val_tmp, ""));
			case TNOT:
				return CGValue(context.builder->CreateNot(val_tmp, ""));
		}
	}

	CGERR_Unknown_Unary_Operation(context);
	CGERR_setLineNum(context, getLine(this), getFile(this));
	CGERR_showAllMsg(context);
	return CGValue();
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
			 uint64_t size, int lineno, char *file_name)
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
					CGERR_setLineNum(context, lineno, file_name);
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
							  Type *variable_type, Value *variable, int lineno, char *file_name)
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
				return context.builder->CreateIntCast(value, variable_type, true);
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
					&& isArrayType(variable_type)) {
			if (!context.currentBlock()) {
				ArrayType *val_elem_type = dyn_cast<ArrayType>(value_type);
				ArrayType *var_elem_type = dyn_cast<ArrayType>(variable_type);
				if (val_elem_type->getNumElements()
					!= var_elem_type->getNumElements()) {
					value = doAlignArray(context, dyn_cast<Constant>(value),
										 var_elem_type->getArrayElementType(),
										 var_elem_type->getNumElements(), lineno, file_name);
				}
			} else {
				assert(variable);
				context.builder->CreateMemCpy(variable,
											  getLoadOperand(context, value, true),
											  getSizeOfJIT(value_type),
											  getAlignOfJIT(value_type), false);
				value = NULL;
			}

			return value;
		} else if (isArrayType(value_type)
					&& isPointerType(variable_type)
					&& !context.currentBlock()) {
			return value;
		} else if (isArrayType(value_type) && isPointerType(variable_type)) {
			val_tmp = context.builder->CreateConstInBoundsGEP2_32(getLoadOperand(context, value, true), 0, 0, "");
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

CGValue
NAssignmentExpr::codeGen(CodeGenContext& context)
{
	Value *lhs;
	Value *rhs;
	Type *integer_type;

	context.setLValue();
	lhs = lval.codeGen(context);
	context.resetLValue();

	if (!isPointer(lhs)
		|| typeid(lval) == typeid(NBinaryExpr)
		|| (typeid(lval) == typeid(NPrefixExpr) && (((NPrefixExpr&)lval).op == TINC || ((NPrefixExpr&)lval).op == TDEC))
		|| typeid(lval) == typeid(NIncDecExpr)) {
		CGERR_Unassignable_LValue(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	if (isIntegerPointer(lhs)) {
		integer_type = dyn_cast<IntegerType>(lhs->getType()->getPointerElementType());
		context.current_bit_width = integer_type->getIntegerBitWidth();
	}
	rhs = rval.codeGen(context);
	context.current_bit_width = 0;

	rhs = NAssignmentExpr::doAssignCast(context, rhs,
										lhs->getType()->getPointerElementType(), lhs,
										getLine(this), getFile(this));

	if (!rhs) {
		return CGValue(rhs);
	}

	context.builder->CreateStore(rhs, lhs, false);

	return CGValue(rhs);
}

CGValue
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
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	if (!struct_type->getStructName().substr(0, strlen(UNION_PREFIX)).compare(UNION_PREFIX)) { // prefix is "union." ?
		is_union_flag = true;
		if (!context.getUnion(struct_type->getStructName())) {
			CGERR_Invalid_Use_Of_Incompelete_Type(context, struct_type->getStructName().str().c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		union_map = *context.getUnion(struct_type->getStructName());
	} else {
		if (!context.getStruct(struct_type->getStructName())) {
			CGERR_Invalid_Use_Of_Incompelete_Type(context, struct_type->getStructName().str().c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}

		map = *context.getStruct(struct_type->getStructName());
	}

	if (is_union_flag) {
		if (union_map.find(field_name.name) != union_map.end()) {
			ret = context.builder->CreateBitCast(struct_value, union_map[field_name.name]->getPointerTo(),
												 "");
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	} else {
		if (map.find(field_name.name) != map.end()) {
			ret = context.builder->CreateStructGEP(struct_value, map[field_name.name], "");
		} else {
			CGERR_Failed_To_Find_Field_Name(context, field_name.name.c_str());
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
	}

	return CGValue(codeGenLoadValue(context, ret));
}

CGValue
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
										getLine(this), getFile(this));

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
		ret = context.builder->CreateInBoundsGEP(context.builder->CreateLoad(array_value),
												 idx, "");
	} else {
		CGERR_Get_Non_Array_Element(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	return CGValue(codeGenLoadValue(context, ret));
}

CGValue
NIncDecExpr::codeGen(CodeGenContext& context)
{
	Type *val_type;
	Value *val_tmp;
	Value *val_ptr;
	Value *rhs;

	val_tmp = operand.codeGen(context);
	val_type = val_tmp->getType();

	if (val_type->isPointerTy()) {
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

		return CGValue(val_tmp);
	}

	if (!(val_ptr = getLoadOperand(context, val_tmp, false))) {
		CGERR_Inc_Dec_Unassignable_Value(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
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

	return CGValue(val_tmp);
}

void
NCondExpr::castCompare(CodeGenContext& context,
					   llvm::BasicBlock *lhs_true, llvm::Value *&lhs,
					   llvm::BasicBlock *lhs_else, llvm::Value *&rhs)
{
	if (lhs->getType() == rhs->getType()) return;

	if (lhs->getType()->isIntegerTy()
		&& rhs->getType()->isIntegerTy()) { // int to int
		if (getBitWidth(rhs->getType())
			> getBitWidth(lhs->getType())) {
			setBlock(lhs_true);
			lhs = context.builder->CreateIntCast(lhs, rhs->getType(), true);
			return;
		} else {
			setBlock(lhs_else);
			rhs = context.builder->CreateIntCast(rhs, lhs->getType(), true);
			return;
		}
	} else if (lhs->getType()->isFloatingPointTy()
				&& rhs->getType()->isFloatingPointTy()) {
		if (rhs->getType()->getTypeID()
			> lhs->getType()->getTypeID()) {
			setBlock(lhs_true);
			lhs = context.builder->CreateFPCast(lhs, rhs->getType());
			return;
		} else {
			setBlock(lhs_else);
			rhs = context.builder->CreateFPCast(rhs, lhs->getType());
			return;
		}
	} else if (lhs->getType()->isFloatingPointTy()
				&& rhs->getType()->isIntegerTy()) {
		setBlock(lhs_else);
		rhs = context.builder->CreateSIToFP(rhs, lhs->getType());
		return;
	} else if (lhs->getType()->isIntegerTy()
				&& rhs->getType()->isFloatingPointTy()) {
		setBlock(lhs_true);
		lhs = context.builder->CreateSIToFP(lhs, rhs->getType());
		return;
	} else {
		setBlock(lhs_else);
		rhs = context.builder->CreateBitCast(rhs, lhs->getType());
		return;
	}

	return;
}

#define _lhs_getParent_O2_ (dyn_cast<Instruction>(lhs) ? \
							dyn_cast<Instruction>(lhs)->getParent() :\
							lhs_true) // only use in condition expression
#define _rhs_getParent_O2_ (dyn_cast<Instruction>(rhs) ? \
							dyn_cast<Instruction>(rhs)->getParent() :\
							lhs_else) // only use in condition expression

CGValue
NCondExpr::codeGen(CodeGenContext& context)
{
	Value *cond_val;
	Value *lhs;
	Value *rhs;
	BasicBlock *lhs_true;
	BasicBlock *lhs_else;
	BasicBlock *lhs_end;
	BasicBlock *orig_block;
	BasicBlock *orig_end_block;
	PHINode *phi_node;

	if (context.isLValue()) {
		context.resetLValue();
		cond_val = context.builder->CreateIsNotNull(cond.codeGen(context));
		context.setLValue();
	} else {
		cond_val = context.builder->CreateIsNotNull(cond.codeGen(context));
	}

	orig_block = context.currentBlock();
	orig_end_block = context.current_end_block;

	lhs_true = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								  context.current_end_block);
	lhs_else = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								  context.current_end_block);
	lhs_end = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								 context.current_end_block);

	context.current_end_block = lhs_end;
	context.builder->CreateCondBr(cond_val, lhs_true, lhs_else);

	setBlock(lhs_true);
	lhs = if_true.codeGen(context);
	context.builder->CreateBr(lhs_end);

	setBlock(lhs_else);
	rhs = if_else.codeGen(context);
	context.builder->CreateBr(lhs_end);

	if (!context.currentBlock()->getTerminator()) {
		context.builder->CreateBr(lhs_end);
	}
	castCompare(context, lhs_true, lhs, lhs_else, rhs);
	setBlock(lhs_end);
	
	phi_node = context.builder->CreatePHI(lhs->getType(), 2);

	phi_node->addIncoming(lhs, _lhs_getParent_O2_);
	phi_node->addIncoming(rhs, _rhs_getParent_O2_);

	context.current_end_block = orig_end_block;

	return CGValue(phi_node);
}

CGValue
NCompoundExpr::codeGen(CodeGenContext& context)
{
	CGValue ret = first.codeGen(context);

	ret.pushExp(second.codeGen(context));

	return ret;
}
