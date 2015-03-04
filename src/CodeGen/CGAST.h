#ifndef _GENCODE_H_
#define _GENCODE_H_

#include <stack>
#include <typeinfo>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CallSite.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>
#include "../ErrorMsg/EMCore.h"

#define STRUCT_PREFIX ("struct.")
#define UNION_PREFIX ("union.")

using namespace llvm;

class NBlock;
class CodeGenContext;

typedef std::map<std::string, Type*> TypeInfoTable;
TypeInfoTable initializeBasicType(CodeGenContext& context);
Value *codeGenLoadValue(CodeGenContext& context, Value *V);
uint64_t getConstantIntExprJIT(Constant *const_expr);

class CodeGenBlock {
public:
    BasicBlock *block;
    Value *returnValue;
    std::map<std::string, Value*> locals;
};

typedef std::map<std::string, unsigned> FieldMap;
typedef std::map<std::string, Type *> UnionFieldMap;

class CodeGenContext {
    std::stack<CodeGenBlock *> blocks;
	std::map<std::string, Value*> globals;
	bool is_lvalue;

public:
    Module *module;
	IRBuilder<> *builder;

	TypeInfoTable types;

	ErrorMessage messages;
	std::map<std::string, FieldMap> structs;
	std::map<std::string, UnionFieldMap> unions;
	int currentBitWidth; // for extra long integer

    CodeGenContext() {
        module = new Module("main", getGlobalContext());
		builder = new IRBuilder<>(getGlobalContext());
		types = initializeBasicType(*this);
		resetLValue();
    }

	~CodeGenContext() {
		delete builder;
	}

    void generateCode(NBlock& root);
    GenericValue runCode();

    std::map<std::string, Value*>& getTopLocals() {
        return blocks.top()->locals;
    }
    std::map<std::string, Value*>& getGlobals() {
        return globals;
    }
    BasicBlock *currentBlock() {
		if (blocks.size()) {
        	return blocks.top()->block;
		}
		return NULL;
    }
	bool isLValue() {
		return is_lvalue;
	}
	void setLValue() {
		is_lvalue = true;
		return;
	}
	void resetLValue() {
		is_lvalue = false;
		return;
	}
    void pushBlock(BasicBlock *block) {
        blocks.push(new CodeGenBlock());
        blocks.top()->returnValue = NULL;
        blocks.top()->block = block;
    }
    void popBlock() {
        CodeGenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }
    void setCurrentReturnValue(Value *value) {
        blocks.top()->returnValue = value;
    }
    Value* getCurrentReturnValue() {
        return blocks.top()->returnValue;
    }
};

#endif
