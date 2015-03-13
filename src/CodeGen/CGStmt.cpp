#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

Value*
NReturnStatement::codeGen(CodeGenContext& context)
{
	Value *tmp_val = expression.codeGen(context);
	Value *ret_val;

	if (tmp_val) {
		ret_val = NAssignmentExpr::doAssignCast(context, expression.codeGen(context),
												context.currentBlock()->getParent()->getReturnType(),
												NULL, ((NStatement*)this)->lineno);
	} else {
		return context.builder->CreateRetVoid();
	}

	return context.builder->CreateRet(ret_val);
}

#define setBlock(b) (context.pushBlock(b), \
					 context.builder->SetInsertPoint(context.currentBlock()))

Value*
NIfStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *orig_end_block;
	BasicBlock *orig_block;
	BasicBlock *if_true_block;
	BasicBlock *if_else_block; // optional
	BasicBlock *end_block;
	Value *cond;

	// record info
	orig_block = context.currentBlock();
	orig_end_block = context.currentEndBlock;

	cond = context.builder->CreateIsNotNull(condition.codeGen(context), "");

	// create & set end block
	end_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								   orig_end_block);
	if (orig_end_block) {
		BranchInst::Create(orig_end_block, end_block); // goto next end
	}
	context.currentEndBlock = end_block;

	if_true_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
									   context.currentEndBlock);
	setBlock(if_true_block);
	if_true->codeGen(context);
	if (!if_true_block->getTerminator()) {
		BranchInst::Create(end_block, if_true_block); // goto end block whether else block is exist ot not (if don't have a terminator)
	}
	context.popBlock();

	if (if_else) {
		if_else_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
										   context.currentEndBlock);
		setBlock(if_else_block);
		if_else->codeGen(context);
		if (!if_else_block->getTerminator()) {
			BranchInst::Create(end_block, if_else_block); // goto end block (if don't have a terminator)
		}
		context.popBlock();

		context.currentEndBlock = orig_end_block; // restore info
		setBlock(end_block); // insert other insts at end block

		return BranchInst::Create(if_true_block, if_else_block, cond, orig_block); // insert branch at original block
	}

	// else (no if_else)

	context.currentEndBlock = orig_end_block; // restore info
	setBlock(end_block); // insert other insts at end block
	if (end_block->getTerminator()) {
		context.builder->SetInsertPoint(end_block->getTerminator());
	}

	return BranchInst::Create(if_true_block, end_block, cond, orig_block);
}
