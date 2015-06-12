#ifndef _GENCODE_H_
#define _GENCODE_H_

#include <stack>
#include <typeinfo>
#include <assert.h>
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
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include "../ErrorMsg/EMCore.h"
#include "CGContainer.h"
#include <time.h>

#define STRUCT_PREFIX ("struct.")
#define UNION_PREFIX ("union.")
#define ANON_POSTFIX ("anon")

using namespace llvm;

class NBlock;
class CodeGenContext;

typedef std::map<std::string, Type*> TypeInfoTable;
TypeInfoTable initializeBasicType(CodeGenContext& context);
CGValue codeGenLoadValue(CodeGenContext& context, Value *V);
uint64_t getConstantIntExprJIT(Constant *const_expr);

typedef std::map<std::string, int> FieldMap;
typedef std::map<std::string, Type *> UnionFieldMap;

typedef struct {
	TypeInfoTable local_types;
    std::map<std::string, Value*> locals;
	std::map<std::string, FieldMap> structs;
	std::map<std::string, UnionFieldMap> unions;
} BlockLocalContext;

class CodeGenBlock {
public:
    BasicBlock *block;
    Value *returnValue;

	TypeInfoTable local_types;
    std::map<std::string, Value*> locals;
	std::map<std::string, FieldMap> structs;
	std::map<std::string, UnionFieldMap> unions;
};

class CodeGenContext {
    std::stack<CodeGenBlock *> blocks;
	std::map<std::string, Value*> globals;
	std::map<std::string, BasicBlock*> labels;
	bool is_lvalue;

public:
    Module *module;
	IRBuilder<> *builder;

	TypeInfoTable types;

	ErrorMessage messages;
	std::map<std::string, FieldMap> structs;
	std::map<std::string, UnionFieldMap> unions;
	int current_bit_width; // for extra long integer
	BasicBlock *current_end_block; // used for branch
	BasicBlock *current_break_block;
	BasicBlock *current_continue_block;
	std::string current_namespace;
	Function *global_constructor;

	int in_param_flag = 0;

    CodeGenContext() {
        module = new Module("main", getGlobalContext());
		builder = new IRBuilder<>(getGlobalContext());
		types = initializeBasicType(*this);
		current_bit_width = 0;
		current_end_block = NULL;
		current_break_block = NULL;
		current_continue_block = NULL;
		current_namespace = "";
		global_constructor = NULL;
		resetLValue();
    }

	~CodeGenContext() {
		delete builder;
	}

	string
	getRandomString(int length);

	void setGlobalConstructor();

	void terminateGlobalConstructor();

    void generateCode(NBlock& root);
    GenericValue runCode();

    std::map<std::string, Value*>& getTopLocals();

    void setTopLocals(std::map<std::string, Value*> locals);

    std::map<std::string, Value*> copyTopLocals();

    std::map<std::string, Value*>& getGlobals();

	BasicBlock *getLabel(std::string name);

	FieldMap *getStruct(std::string name);

	void setStruct(std::string name, FieldMap map);

	UnionFieldMap *getUnion(std::string name);

	void setUnion(std::string name, UnionFieldMap map);

	Type *getType(std::string name);

	void setType(std::string name, Type *type);

	inline const std::string
	formatName(const std::string &name)
	{
		return current_namespace + name;
	}

	void setLabel(std::string name, BasicBlock *block);

    BasicBlock *currentBlock();

	BlockLocalContext backupLocalContext();

	void restoreLocalContext(BlockLocalContext context);

    TerminatorInst *currentTerminator();

	bool isLValue();
	void setLValue();
	void resetLValue();

    void pushBlock(BasicBlock *block);
    void popBlock();
	void popAllBlock();

    void setCurrentReturnValue(Value *value);
    Value* getCurrentReturnValue();
};

#endif
