#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define setBlock(b) (context.pushBlock(b), \
					 context.builder->SetInsertPoint(context.currentBlock()))

Value *
NBlock::codeGen(CodeGenContext& context)
{
	BlockLocalContext local_context = context.backupLocalContext();
	StatementList::const_iterator it;
	Value *last = NULL;

	for (it = statements.begin(); it != statements.end(); it++) {
		if (*it) {
			last = (**it).codeGen(context);
		}
	}
	context.restoreLocalContext(local_context);

	return last;
}

Value *
NReturnStatement::codeGen(CodeGenContext& context)
{
	Value *tmp_val = expression.codeGen(context);
	Value *ret_val;

	if (tmp_val) {
		ret_val = NAssignmentExpr::doAssignCast(context, tmp_val,
												context.currentBlock()->getParent()->getReturnType(),
												NULL, ((NStatement*)this)->lineno);
	} else {
		return context.builder->CreateRetVoid();
	}

	return context.builder->CreateRet(ret_val);
}

Value *
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

	if (!context.currentBlock()->getTerminator()) {
		BranchInst::Create(end_block, context.currentBlock()); // goto end block whether else block is exist ot not (if don't have a terminator)
	}
	context.popBlock();

	if (if_else) {
		if_else_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
										   context.currentEndBlock);
		setBlock(if_else_block);
		if_else->codeGen(context);
		if (!context.currentBlock()->getTerminator()) {
			BranchInst::Create(end_block, context.currentBlock()); // goto end block (if don't have a terminator)
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

Value *
NLabelStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *labeled_block;

	if (!(labeled_block = context.getLabel(label_name))) {
		labeled_block = BasicBlock::Create(getGlobalContext(), "",
										   context.currentBlock()->getParent(),
										   context.currentEndBlock);
		context.setLabel(label_name, labeled_block);
	} else {
		labeled_block->moveAfter(context.currentBlock());
	}

	if (!context.currentBlock()->getTerminator()) {
		BranchInst::Create(labeled_block, context.currentBlock());
	}

	setBlock(labeled_block);

	return statement.codeGen(context);
}

Value *
NGotoStatement::codeGen(CodeGenContext& context)
{
	BranchInst *br_inst;
	BasicBlock *dest_block;

	if (!(dest_block = context.getLabel(label_name))) {
		dest_block = BasicBlock::Create(getGlobalContext(), "",
									    context.currentBlock()->getParent(),
									    context.currentEndBlock);
		context.setLabel(label_name, dest_block);
	}
	br_inst = context.builder->CreateBr(dest_block);
	setBlock(BasicBlock::Create(getGlobalContext(), "",
								context.currentBlock()->getParent(),
								context.currentEndBlock));

	return br_inst;
}
