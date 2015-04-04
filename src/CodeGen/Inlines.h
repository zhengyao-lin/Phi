#ifndef _INLINES_H_
#define _INLINES_H_

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/APFloat.h>
#include "CGAST.h"

inline double
getConstantDouble(Value *C)
{
	return ((ConstantFP *)C)->getValueAPF().convertToDouble();
}
inline uint64_t
getConstantInt(Value *C)
{
	return *((ConstantInt *)C)->getValue().getRawData();
}
inline string
getConstantIntStr(Value *C)
{
	return ((ConstantInt *)C)->getValue().toString(10, true);
}
inline bool
isConstantFP(Value *V)
{
	return dyn_cast<ConstantFP>(V) != NULL;
}
inline bool
isConstantInt(Value *V)
{
	return dyn_cast<ConstantInt>(V) != NULL;
}
inline bool
iSConstantDataArray(Value *V)
{
	return dyn_cast<ConstantDataArray>(V) != NULL;
}
inline bool
iSConstantExpr(Value *V)
{
	return dyn_cast<ConstantExpr>(V) != NULL;
}
inline bool
iSConstantPointerNull(Value *V)
{
	return dyn_cast<ConstantPointerNull>(V) != NULL;
}
inline bool
isConstant(Value *V)
{
	return isConstantFP(V) || isConstantInt(V)
			|| iSConstantDataArray(V) || iSConstantExpr(V)
			|| iSConstantPointerNull(V);
}
inline bool
isFunctionPointer(Value *V)
{
	return (V->getType()->getTypeID() == Type::PointerTyID
			&& V->getType()->getPointerElementType()->getTypeID() == Type::FunctionTyID);
}
inline bool
isStructPointer(Value *V)
{
	return (V->getType()->getTypeID() == Type::PointerTyID
			&& V->getType()->getPointerElementType()->getTypeID() == Type::StructTyID);
}
inline bool
isArrayPointer(Value *V)
{
	return (V->getType()->getTypeID() == Type::PointerTyID
			&& V->getType()->getPointerElementType()->getTypeID() == Type::ArrayTyID);
}
inline bool
isArrayPointerType(Type *T)
{
	return (T->getTypeID() == Type::PointerTyID
			&& T->getPointerElementType()->getTypeID() == Type::ArrayTyID);
}
inline bool
isArrayType(Type *T)
{
	return T->getTypeID() == Type::ArrayTyID;
}
inline bool
isArray(Value *V)
{
	return V->getType()->getTypeID() == Type::ArrayTyID;
}
inline bool
isPointerType(Type *T)
{
	return T->getTypeID() == Type::PointerTyID;
}
inline bool
isPointer(Value *V)
{
	return isPointerType(V->getType());
}
inline bool
isPointerPointer(Value *V)
{
	return (V->getType()->getTypeID() == Type::PointerTyID
			&& V->getType()->getPointerElementType()->getTypeID() == Type::PointerTyID);
}
inline bool
isPointerPointerType(Type *T)
{
	return (T->getTypeID() == Type::PointerTyID
			&& T->getPointerElementType()->getTypeID() == Type::PointerTyID);
}
inline bool
isIntegerPointer(Value *V)
{
	return (V->getType()->getTypeID() == Type::PointerTyID
			&& V->getType()->getPointerElementType()->getTypeID() == Type::IntegerTyID);
}
inline bool
isIntegerPointerType(Type *T)
{
	return (T->getTypeID() == Type::PointerTyID
			&& T->getPointerElementType()->getTypeID() == Type::IntegerTyID);
}
inline bool
isBooleanType(Type *T)
{
	return (T->getTypeID() == Type::IntegerTyID
			&& dyn_cast<IntegerType>(T)->getIntegerBitWidth() == 1);
}
inline bool
isBoolean(Value *V)
{
	return (V->getType()->getTypeID() == Type::IntegerTyID
			&& dyn_cast<IntegerType>(V->getType())->getIntegerBitWidth() == 1);
}
inline bool
isInt32Type(Type *T)
{
	return (T->getTypeID() == Type::IntegerTyID
			&& dyn_cast<IntegerType>(T)->getIntegerBitWidth() == 32);
}
inline bool
isFunctionType(Type *T)
{
	return dyn_cast<FunctionType>(T) != NULL;
}
inline bool
isVoid(Value *V)
{
	return V->getType()->getTypeID() == Type::VoidTyID;
}
inline bool
isVoidType(Type *T)
{
	return T->getTypeID() == Type::VoidTyID;
}
inline unsigned
getBitWidth(Type *T)
{
	return dyn_cast<IntegerType>(T)->getIntegerBitWidth();
}
inline bool
isSameType(Type *T1, Type *T2)
{
	/*if (T1->getTypeID() == T2->getTypeID()) {
		if (T1->getTypeID() == Type::IntegerTyID) {
			if (T1->getIntegerBitWidth() == T2->getIntegerBitWidth()) {
				return true;
			}
		} else if (T1->getTypeID() == Type::PointerTyID) {
			if (isSameType(T1->getPointerElementType(), T2->getPointerElementType())) {
				return true;
			}
		} else if (T1->getTypeID() == Type::ArrayTyID) {
			return false; // do not assign array without memcpy
		} else {
			return true;
		}
	}*/

	return T1 == T2;
}
inline bool
hasSameType(Value *V1, Value *V2)
{
	return isSameType(V1->getType(), V2->getType());
}
inline Value *
FPCast_opt(CodeGenContext& context, Value *V, Type *FPT) // NOTICE: size of FPT must longer than V's
{
	if (isConstantFP(V)) {
		return ConstantFP::get(FPT, 
								getConstantDouble(V));
	} else {
		return new FPExtInst(V, FPT, "",
							  context.currentBlock());
	}
}
inline Value *
IntCast_opt(CodeGenContext& context, Value *V, Type *IntT)
{
	if (isConstantInt(V)) {
		return ConstantInt::get(IntT, 
								getConstantInt(V));
	} else {
		return new SExtInst(V, IntT, "",
							 context.currentBlock());
	}
}
inline Value *
IntToFPCast_opt(CodeGenContext& context, Value *IV, Type *FPT)
{
	if (isConstantInt(IV)) {
		return ConstantFP::get(FPT, 
								APIntOps::RoundSignedAPIntToDouble(((ConstantInt *)IV)->getValue()));
	} else {
		return new SIToFPInst(IV, FPT, "",
							   context.currentBlock());
	}
}
inline Value *
FPToIntCast_opt(CodeGenContext& context, Value *V, Type *IntT)
{
	if (isConstantFP(V)) {
		return ConstantInt::get(IntT, 
								 getConstantDouble(V));
	} else {
		return new FPToSIInst(V, IntT, "",
							   context.currentBlock());
	}
}

inline uint64_t
getSizeOfJIT(Type *T)
{
	return getConstantIntExprJIT(ConstantExpr::getSizeOf(T));
}

inline uint64_t
getAlignOfJIT(Type *T)
{
	return getConstantIntExprJIT(ConstantExpr::getAlignOf(T));
}

#endif
