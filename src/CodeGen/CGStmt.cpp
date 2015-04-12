#include "AST/Node.h"
#include "CGAST.h"
#include "CGErr.h"
#include "Grammar/Parser.hpp"
#include "Inlines.h"

#define setBlock(b) (context.pushBlock(b), \
					 context.builder->SetInsertPoint(context.currentBlock()))
#define getLine(p) (((NStatement *)this)->lineno)
#define getFile(p) (((NStatement *)this)->file_name)

CGValue
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

	return CGValue(last);
}

CGValue
NReturnStatement::codeGen(CodeGenContext& context)
{
	Value *tmp_val = expression.codeGen(context);
	Value *ret_val;

	if (tmp_val) {
		ret_val = NAssignmentExpr::doAssignCast(context, tmp_val,
												context.currentBlock()->getParent()->getReturnType(),
												NULL, getLine(this), getFile(this));
	} else {
		return CGValue(context.builder->CreateRetVoid());
	}

	return CGValue(context.builder->CreateRet(ret_val));
}

CGValue
NWhileStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *orig_end_block;
	BasicBlock *orig_break_block;
	BasicBlock *orig_continue_block;
	BasicBlock *orig_block;
	BasicBlock *cond_block;
	BasicBlock *while_true_block;
	BasicBlock *end_block;
	Value *cond;

	orig_block = context.currentBlock();
	orig_end_block = context.current_end_block;
	orig_break_block = context.current_break_block;
	orig_continue_block = context.current_continue_block;

	cond_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
									orig_end_block);
	BranchInst::Create(cond_block, orig_block); // auto jump to cond block
	setBlock(cond_block);

	cond = context.builder->CreateIsNotNull(condition.codeGen(context), "");
	orig_block = context.currentBlock();

	end_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								   orig_end_block);
	context.current_end_block = end_block;
	context.current_break_block = end_block;
	context.current_continue_block = cond_block;

	while_true_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
										  context.current_end_block);

	setBlock(while_true_block);
	while_true->codeGen(context);
	context.builder->CreateBr(cond_block); // rejudge

	context.popBlock();

	context.current_end_block = orig_end_block; // restore info
	context.current_break_block = orig_break_block;
	context.current_continue_block = orig_continue_block;
	setBlock(end_block); // insert other insts at end block

	return CGValue(BranchInst::Create(while_true_block, end_block, cond, orig_block));
}

CGValue
NForStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *orig_end_block;
	BasicBlock *orig_break_block;
	BasicBlock *orig_continue_block;
	BasicBlock *orig_block;
	BasicBlock *cond_block;
	BasicBlock *for_true_block;
	BasicBlock *tail_block;
	BasicBlock *end_block;
	Value *cond;

	initializer.codeGen(context);
	orig_block = context.currentBlock();
	orig_end_block = context.current_end_block;
	orig_break_block = context.current_break_block;
	orig_continue_block = context.current_continue_block;

	cond_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
									orig_end_block);
	BranchInst::Create(cond_block, orig_block); // auto jump to cond block
	setBlock(cond_block);

	if (cond = condition.codeGen(context)) {
		cond = context.builder->CreateIsNotNull(condition.codeGen(context), "");
	}
	orig_block = context.currentBlock();

	end_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								   orig_end_block);
	context.current_end_block = end_block;
	for_true_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
										context.current_end_block);
	tail_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
									context.current_end_block);
	context.current_break_block = end_block;
	context.current_continue_block = tail_block;

	setBlock(for_true_block);
	for_true->codeGen(context);
	context.builder->CreateBr(tail_block); // goto tail

	setBlock(tail_block);
	tail.codeGen(context);
	context.builder->CreateBr(cond_block); // rejudge

	context.popBlock();

	context.current_end_block = orig_end_block; // restore info
	context.current_break_block = orig_break_block;
	context.current_continue_block = orig_continue_block;
	setBlock(end_block); // insert other insts at end block

	if (!cond) {
		return BranchInst::Create(for_true_block, orig_block);
	}

	return CGValue(BranchInst::Create(for_true_block, end_block, cond, orig_block));
}

CGValue
NIfStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *orig_end_block;
	BasicBlock *orig_block;
	BasicBlock *if_true_block;
	BasicBlock *if_else_block; // optional
	BasicBlock *end_block;
	Value *cond;

	cond = context.builder->CreateIsNotNull(condition.codeGen(context), "");

	// record info
	orig_block = context.currentBlock();
	orig_end_block = context.current_end_block;

	// create & set end block
	end_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
								   orig_end_block);
	context.current_end_block = end_block;

	if_true_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
									   context.current_end_block);
	setBlock(if_true_block);
	if_true->codeGen(context);

	if (!context.currentBlock()->getTerminator()) {
		BranchInst::Create(end_block, context.currentBlock()); // goto end block whether else block is exist ot not (if don't have a terminator)
	}
	context.popBlock();

	if (if_else) {
		if_else_block = BasicBlock::Create(getGlobalContext(), "", orig_block->getParent(),
										   context.current_end_block);
		setBlock(if_else_block);
		if_else->codeGen(context);
		if (!context.currentBlock()->getTerminator()) {
			BranchInst::Create(end_block, context.currentBlock()); // goto end block (if don't have a terminator)
		}
		context.popBlock();

		context.current_end_block = orig_end_block; // restore info
		setBlock(end_block); // insert other insts at end block

		return CGValue(BranchInst::Create(if_true_block, if_else_block, cond, orig_block)); // insert branch at original block
	}

	// else (no if_else)

	context.current_end_block = orig_end_block; // restore info
	setBlock(end_block); // insert other insts at end block
	if (end_block->getTerminator()) {
		context.builder->SetInsertPoint(end_block->getTerminator());
	}

	return CGValue(BranchInst::Create(if_true_block, end_block, cond, orig_block));
}

CGValue
NLabelStatement::codeGen(CodeGenContext& context)
{
	BasicBlock *labeled_block;

	if (!(labeled_block = context.getLabel(label_name))) {
		labeled_block = BasicBlock::Create(getGlobalContext(), "",
										   context.currentBlock()->getParent(),
										   context.current_end_block);
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

CGValue
NGotoStatement::codeGen(CodeGenContext& context)
{
	BranchInst *br_inst;
	BasicBlock *dest_block;

	if (!(dest_block = context.getLabel(label_name))) {
		dest_block = BasicBlock::Create(getGlobalContext(), "",
									    context.currentBlock()->getParent(),
									    context.current_end_block);
		context.setLabel(label_name, dest_block);
	}
	br_inst = context.builder->CreateBr(dest_block);
	setBlock(BasicBlock::Create(getGlobalContext(), "",
								context.currentBlock()->getParent(),
								context.current_end_block));

	return CGValue(br_inst);
}

CGValue
NJumpStatement::codeGen(CodeGenContext& context)
{
	BranchInst *br_inst;

	if (is_continue) {
		if (!context.current_continue_block) {
			CGERR_Continue_Without_Iteration(context);
			CGERR_setLineNum(context, getLine(this), getFile(this));
			CGERR_showAllMsg(context);
			return CGValue();
		}
		br_inst = context.builder->CreateBr(context.current_continue_block);
		setBlock(BasicBlock::Create(getGlobalContext(), "",
							context.currentBlock()->getParent(),
							context.current_end_block));
		return CGValue(br_inst);
	}

	if (!context.current_break_block) {
		CGERR_Break_Without_Iteration(context);
		CGERR_setLineNum(context, getLine(this), getFile(this));
		CGERR_showAllMsg(context);
		return CGValue();
	}

	br_inst = context.builder->CreateBr(context.current_break_block);
	setBlock(BasicBlock::Create(getGlobalContext(), "",
							context.currentBlock()->getParent(),
							context.current_end_block));

	return CGValue(br_inst);
}
