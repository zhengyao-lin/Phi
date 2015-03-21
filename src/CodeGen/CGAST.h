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

typedef std::map<std::string, unsigned> FieldMap;
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
	std::string current_namespace;

    CodeGenContext() {
        module = new Module("main", getGlobalContext());
		builder = new IRBuilder<>(getGlobalContext());
		types = initializeBasicType(*this);
		current_bit_width = 0;
		current_end_block = NULL;
		current_namespace = "";
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
    void setTopLocals(std::map<std::string, Value*> locals) {
		if (currentBlock()) {
        	blocks.top()->locals = locals;
		}
		return;
    }
    std::map<std::string, Value*> copyTopLocals() {
		if (currentBlock()) {
        	return blocks.top()->locals;
		}
		return *new std::map<std::string, Value*>();
    }
    std::map<std::string, Value*>& getGlobals() {
        return globals;
    }
	BasicBlock *getLabel(std::string name) {
		if (labels.find(name) == labels.end()) {
			return NULL;
		}
		return labels[name];
	}

	FieldMap *getStruct(std::string name) {
		if (currentBlock()
			&& blocks.top()->structs.find(formatName(name)) != blocks.top()->structs.end()) {
			return &blocks.top()->structs[formatName(name)];
		}

		if (currentBlock()
			&& blocks.top()->structs.find(name) != blocks.top()->structs.end()) {
			return &blocks.top()->structs[name];
		}

		if (structs.find(formatName(name)) != structs.end()) {
			return &structs[formatName(name)];
		}

		if (structs.find(name) != structs.end()) {
			return &structs[name];
		}

		return NULL;
	}

	void setStruct(std::string name, FieldMap map) {
		if (currentBlock()) {
			blocks.top()->structs[name] = map;
		} else {
			std::cout << "set: " << name << endl;
			structs[name] = map;
		}

		return;
	}

	UnionFieldMap *getUnion(std::string name) {
		if (currentBlock()
			&& blocks.top()->unions.find(formatName(name)) != blocks.top()->unions.end()) {
			return &blocks.top()->unions[formatName(name)];
		}

		if (currentBlock()
			&& blocks.top()->unions.find(name) != blocks.top()->unions.end()) {
			return &blocks.top()->unions[name];
		}

		if (unions.find(formatName(name)) != unions.end()) {
			return &unions[formatName(name)];
		}

		if (unions.find(name) != unions.end()) {
			return &unions[name];
		}

		return NULL;
	}

	void setUnion(std::string name, UnionFieldMap map) {
		if (currentBlock()) {
			blocks.top()->unions[name] = map;
		} else {
			unions[name] = map;
		}

		return;
	}

	Type *getType(std::string name) {
		if (currentBlock()
			&& blocks.top()->local_types.find(formatName(name)) != blocks.top()->local_types.end()) {
			return blocks.top()->local_types[formatName(name)];
		}

		if (currentBlock()
			&& blocks.top()->local_types.find(name) != blocks.top()->local_types.end()) {
			return blocks.top()->local_types[name];
		}

		if (types.find(formatName(name)) != types.end()) {
			return types[formatName(name)];
		}

		if (types.find(name) != types.end()) {
			return types[name];
		}

		return NULL;
	}

	void setType(std::string name, Type *type) {
		if (currentBlock()) {
			blocks.top()->local_types[name] = type;
		} else {
			types[name] = type;
		}

		return;
	}

	inline std::string
	formatName(std::string name)
	{
		return current_namespace + name;
	}

	void setLabel(std::string name, BasicBlock *block) {
		labels[name] = block;
		return;
	}
    BasicBlock *currentBlock() {
		if (blocks.size()) {
        	return blocks.top()->block;
		}
		return NULL;
    }
	BlockLocalContext backupLocalContext() {
		BlockLocalContext ret;

		if (currentBlock()) {
			ret.local_types = blocks.top()->local_types;
			ret.locals = blocks.top()->locals;
			ret.structs = blocks.top()->structs;
			ret.unions = blocks.top()->unions;
		}

		return ret;
	}
	void restoreLocalContext(BlockLocalContext context) {
		if (currentBlock()) {
			blocks.top()->local_types = context.local_types;
			blocks.top()->locals = context.locals;
			blocks.top()->structs = context.structs;
			blocks.top()->unions = context.unions;
		}

		return;
	}
    TerminatorInst *currentTerminator() {
		if (blocks.size()) {
        	return blocks.top()->block->getTerminator();
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
		CodeGenBlock *newb = new CodeGenBlock();

        newb->returnValue = NULL;
        newb->block = block;
		if (currentBlock()) { // inherit locals
			BlockLocalContext context = backupLocalContext();
			blocks.push(newb);
			restoreLocalContext(context);

			return;
		}

		blocks.push(newb);
		return;
    }
    void popBlock() {
        CodeGenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }
	void popAllBlock() {
		while (currentBlock()) popBlock();
		return;
	}
    void setCurrentReturnValue(Value *value) {
        blocks.top()->returnValue = value;
    }
    Value* getCurrentReturnValue() {
        return blocks.top()->returnValue;
    }
};

#endif
