#ifndef _NODE_H_
#define _NODE_H_

#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/GlobalValue.h>
#include "../CodeGen/CGContainer.h"

class CodeGenContext;
class NStatement;
class NExpression;
class NIdentifier;
class NVariableDecl;
class NParamDecl;
class NStructDecl;
class NUnionDecl;
class NSpecifier;
class NFunctionDecl;
class DeclInfo;
class Declarator;

typedef std::vector<NStatement*> StatementList;
typedef std::vector<NExpression*> ExpressionList;
typedef std::vector<NVariableDecl*> VariableList;
typedef std::vector<NParamDecl*> ParamList;
typedef std::vector<NExpression*> ArrayDim;
typedef std::vector<NSpecifier*> DeclSpecifier;
typedef std::vector<Declarator *> DeclaratorList;

class Node {
public:
	int lineno = -1;
	char *file_name = NULL;
	virtual ~Node() {}
	virtual CGValue codeGen(CodeGenContext& context) { return CGValue(); }
};
class NStatement : public Node {
public:
	int lineno = -1;
	char *file_name = NULL;
	virtual ~NStatement() {}
};
class NExpression : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	virtual ~NExpression() {}
};

class NCompoundExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression &first;
	NExpression &second;

	NCompoundExpr(NExpression &first, NExpression &second) :
	first(first), second(second) { }

	virtual ~NCompoundExpr()
	{
		delete &first;
		delete &second;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NIdentifier : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string& name;

	NIdentifier(std::string& name) :
	name(name) { }

	virtual ~NIdentifier()
	{
		delete &name;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class DeclInfo {
public:
	llvm::Type *type;
	NIdentifier *id = NULL;
	NExpression *expr = NULL;
	ParamList *arguments = NULL;

	DeclInfo(llvm::Type *type, NIdentifier *id) :
	type(type), id(id) { }

	virtual ~DeclInfo()
	{
		delete id;
	}
};

// Declarator
class Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	virtual ~Declarator() {}
	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
	{
		return new DeclInfo(base_type, NULL);
	}
};

class IdentifierDeclarator : public Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	NIdentifier &id;

	IdentifierDeclarator(NIdentifier& id) :
	id(id) { }

	virtual ~IdentifierDeclarator()
	{
		delete &id;
	}

	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type);
};

class ArrayDeclarator : public Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	Declarator& decl;
	ArrayDim& array_dim;

	ArrayDeclarator(Declarator& decl, ArrayDim& array_dim) :
	decl(decl), array_dim(array_dim) { }

	virtual ~ArrayDeclarator()
	{
		delete &decl;
		delete &array_dim;
	}

	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type);
};

class PointerDeclarator : public Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	int ptr_dim;
	Declarator& decl;

	PointerDeclarator(int ptr_dim, Declarator& decl) :
	ptr_dim(ptr_dim), decl(decl) { }

	virtual ~PointerDeclarator()
	{
		delete &decl;
	}

	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type);
};

class ParamDeclarator : public Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	Declarator& decl;
	ParamList& arguments;
	bool has_vargs;

	ParamDeclarator(Declarator& decl, ParamList& arguments, bool has_vargs) :
	decl(decl), arguments(arguments), has_vargs(has_vargs) { }

	~ParamDeclarator()
	{
		delete &decl;

		ParamList::const_iterator it;
		for (it = arguments.begin(); it != arguments.end(); it++) {
			delete *it;
		}
		delete &arguments;
	}

	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type);
};

class InitDeclarator : public Declarator {
public:
	int lineno = -1;
	char *file_name = NULL;
	Declarator& decl;
	NExpression *initializer = NULL;

	InitDeclarator(Declarator& decl, NExpression *initializer) :
	decl(decl), initializer(initializer) { }

	virtual ~InitDeclarator()
	{
		delete &decl;
	}

	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type);
};

// Type Specifier
class NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	virtual llvm::Type* getType(CodeGenContext& context);
	virtual ~NType() {}
};

class NDerivedType : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	NType& base;
	int ptr_dim;

	NDerivedType(NType& base, int ptr_dim) :
	base(base), ptr_dim(ptr_dim) { }

	virtual ~NDerivedType()
	{
		delete &base;
	}

	virtual llvm::Type* getType(CodeGenContext& context);
};

class SpecifierSet {
public:
	llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::CommonLinkage;
	NType *type = NULL;

	virtual ~SpecifierSet()
	{
		if (type)
			delete type;
	}
};

class NSpecifier {
public:
	virtual void setSpecifier(SpecifierSet *dest);
	virtual ~NSpecifier() {}
};

class NExternSpecifier : public NSpecifier {
public:
	virtual void setSpecifier(SpecifierSet *dest);
	virtual ~NExternSpecifier() {}
};

class NStaticSpecifier : public NSpecifier {
public:
	virtual void setSpecifier(SpecifierSet *dest);
	virtual ~NStaticSpecifier() {}
};

class NTypeSpecifier : public NSpecifier {
public:
	NType& type;

	NTypeSpecifier(NType& type) :
	type(type) {}

	virtual ~NTypeSpecifier()
	{
		//delete &type;
	}

	virtual void setSpecifier(SpecifierSet *dest);
};

class NIdentifierType : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	NIdentifier& type;
	
	NIdentifierType(NIdentifier &type) :
	type(type) { }

	virtual ~NIdentifierType()
	{
		delete &type;
	}

	virtual llvm::Type* getType(CodeGenContext& context);
};

// Constant
class NVoid : public NExpression {
	int lineno = -1;
	char *file_name = NULL;

	virtual CGValue codeGen(CodeGenContext& context)
	{
		return CGValue();
	}

	virtual ~NVoid() {}
};
class NInteger : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string& value;

	NInteger(std::string& value) :
	value(value) { }

	virtual ~NInteger()
	{
		delete &value;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NChar : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	char value;

	NChar(char value) :
	value(value) { }

	virtual ~NChar()
	{ }

	virtual CGValue codeGen(CodeGenContext& context);
};
class NBoolean : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	bool value;

	NBoolean(bool value) :
	value(value) { }

	virtual ~NBoolean() {}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NDouble : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	double value;

	NDouble(double value) :
	value(value) { }

	virtual ~NDouble() {}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NString : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string value;

	NString(const std::string& value) :
	value(value) { }

	virtual ~NString() {}

	virtual CGValue codeGen(CodeGenContext& context);
};
// Primary Expression End

// Postfix Expression
class NMethodCall : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& func_expr;
	ExpressionList& arguments;

	NMethodCall(NExpression& func_expr, ExpressionList& arguments) :
	func_expr(func_expr), arguments(arguments) { }

	NMethodCall(NExpression& func_expr) :
	func_expr(func_expr), arguments(*new ExpressionList()) { }

	virtual ~NMethodCall()
	{
		delete &func_expr;
		ExpressionList::const_iterator it;
		for (it = arguments.begin();
			 it != arguments.end(); it++) {
			delete *it;
		}
		delete &arguments;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NFieldExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& operand;
	NIdentifier& field_name;

	NFieldExpr(NExpression& operand, NIdentifier& field_name) :
	operand(operand), field_name(field_name) { }

	virtual ~NFieldExpr()
	{
		delete &operand;
		delete &field_name;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NArrayExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& operand;
	NExpression& index;

	NArrayExpr(NExpression& operand, NExpression& index) :
	operand(operand), index(index) { }

	virtual ~NArrayExpr()
	{
		delete &operand;
		delete &index;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};
class NCondExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression &cond;
	NExpression &if_true;
	NExpression &if_else;

	NCondExpr(NExpression &cond, NExpression &if_true, NExpression &if_else) :
	cond(cond), if_true(if_true), if_else(if_else) { }

	virtual ~NCondExpr()
	{
		delete &cond;
		delete &if_true;
		delete &if_else;
	}

	virtual CGValue codeGen(CodeGenContext& context);
	virtual void castCompare(CodeGenContext& context,
							  llvm::BasicBlock *lhs_true, llvm::Value *&lhs,
							  llvm::BasicBlock *lhs_else, llvm::Value *&rhs);
};
// Postfix Expression End

// Binary Expression
class NBinaryExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	int op;
	bool do_not_delete_lval = false;
	NExpression& lval;
	NExpression& rval;

	NBinaryExpr(NExpression& lval, int op, NExpression& rval) :
	lval(lval), rval(rval), op(op) { }

	NBinaryExpr(NExpression& lval, int op, NExpression& rval, bool do_not_delete_lval) :
	lval(lval), rval(rval), op(op), do_not_delete_lval(do_not_delete_lval) { }

	virtual ~NBinaryExpr()
	{
		if (!do_not_delete_lval)
			delete &lval;
		delete &rval;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

// Prefix Expression
class NPrefixExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	int op;
	NType& type;
	NExpression& operand;

	NPrefixExpr(int op, NExpression& operand) :
	op(op), operand(operand),
	type(*new NType()) { }

	NPrefixExpr(NType& type, NExpression& operand) :
	type(type), operand(operand), op(-1) { }

	NPrefixExpr(int op, NType& type) :
	type(type), operand(*new NExpression()), op(op) { }

	virtual ~NPrefixExpr()
	{
		delete &type;
		delete &operand;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NIncDecExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	int op;
	NExpression& operand;

	NIncDecExpr(NExpression& operand, int op) :
	operand(operand), op(op) { }

	virtual ~NIncDecExpr()
	{
		delete &operand;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NAssignmentExpr : public NExpression {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& lval;
	NExpression& rval;

	NAssignmentExpr(NExpression& lval, NExpression& rval) :
	lval(lval), rval(rval) { }

	static llvm::Value *doAssignCast(CodeGenContext& context, llvm::Value *value,
									  llvm::Type *variable_type, llvm::Value *variable, int lineno, char *file_name);

	virtual ~NAssignmentExpr()
	{
		delete &lval;
		delete &rval;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NTypeof : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& operand;
	
	NTypeof(NExpression& operand) :
	operand(operand) { }

	virtual ~NTypeof()
	{
		delete &operand;
	}

	virtual llvm::Type* getType(CodeGenContext& context);
};

class NReturnStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& expression;

	NReturnStatement(NExpression& expression) :
	expression(expression) { }

	virtual ~NReturnStatement()
	{
		delete &expression;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NIfStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression& condition;
	NStatement *if_true;
	NStatement *if_else;

	NIfStatement(NExpression& condition, NStatement *if_true, NStatement *if_else) :
	condition(condition), if_true(if_true), if_else(if_else) { }

	virtual ~NIfStatement()
	{
		delete &condition;
		delete if_true;
		delete if_else;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NWhileStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression &condition;
	NStatement *while_true;

	NWhileStatement(NExpression &condition, NStatement *while_true) :
	condition(condition), while_true(while_true) { }

	virtual ~NWhileStatement()
	{
		delete &condition;
		delete while_true;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NForStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NExpression &initializer;
	NExpression &condition;
	NExpression &tail;
	NStatement *for_true;

	NForStatement(NExpression &initializer, NExpression& condition, NExpression &tail, NStatement *for_true) :
	initializer(initializer), condition(condition), tail(tail), for_true(for_true) { }

	virtual ~NForStatement()
	{
		delete &initializer;
		delete &condition;
		delete &tail;
		delete for_true;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NParamDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NType& type;
	Declarator& decl;

	NParamDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NParamDecl()
	{
		delete &type;
		delete &decl;
	}

	// virtual CGValue codeGen(CodeGenContext& context);
};

class NLabelStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string label_name;
	NStatement& statement;

	NLabelStatement(std::string label_name, NStatement& statement) :
	label_name(label_name), statement(statement) { }

	virtual ~NLabelStatement()
	{
		delete &statement;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NGotoStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string label_name;

	NGotoStatement(std::string label_name) :
	label_name(label_name) { }

	virtual ~NGotoStatement() {}
	virtual CGValue codeGen(CodeGenContext& context);
};

class NJumpStatement : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	bool is_continue;

	NJumpStatement(bool is_continue) :
	is_continue(is_continue) { }

	virtual ~NJumpStatement() {}
	virtual CGValue codeGen(CodeGenContext& context);
};

class NDelegateDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NType& type;
	Declarator& decl;

	NDelegateDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NDelegateDecl()
	{
		delete &type;
		delete &decl;
	}

    virtual CGValue codeGen(CodeGenContext& context);
};

class NVariableDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	DeclSpecifier& var_specifier;
	DeclaratorList *declarator_list;

	SpecifierSet *specifiers;

	NVariableDecl(DeclSpecifier& var_specifier, DeclaratorList *declarator_list) :
	var_specifier(var_specifier), declarator_list(declarator_list) { specifiers = new SpecifierSet(); }

	virtual ~NVariableDecl()
	{
		DeclSpecifier::iterator di;
		for (di = var_specifier.begin();
			 di != var_specifier.end(); di++) {
			delete *di;
		}
		delete &var_specifier;

		DeclaratorList::iterator it;
		for (it = declarator_list->begin();
			 it != declarator_list->end(); it++) {
			delete *it;
		}
		delete declarator_list;

		delete specifiers;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NStructDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NIdentifier& id;
    VariableList *fields;

    NStructDecl(NIdentifier& id, VariableList *fields) :
	id(id), fields(fields) { }

	virtual ~NStructDecl()
	{
		delete &id;

		if (fields) {
			VariableList::const_iterator it;
			for (it = fields->begin(); it != fields->end(); it++) {
				delete *it;
			}
			delete fields;
		}
	}

    virtual CGValue codeGen(CodeGenContext& context);
};

class NUnionDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NIdentifier& id;
	VariableList *fields;

    NUnionDecl(NIdentifier& id, VariableList *fields) :
	id(id), fields(fields) { }

	virtual ~NUnionDecl()
	{
		delete &id;

		if (fields) {
			VariableList::const_iterator it;
			for (it = fields->begin(); it != fields->end(); it++) {
				delete *it;
			}
			delete fields;
		}
	}

    virtual CGValue codeGen(CodeGenContext& context);
};

class NStructType : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	NStructDecl *struct_decl;

    NStructType(NStructDecl *struct_decl) :
	struct_decl(struct_decl) { }

	virtual ~NStructType()
	{
		delete struct_decl;
	}

    virtual llvm::Type* getType(CodeGenContext& context);
};

class NUnionType : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	NUnionDecl *union_decl;

    NUnionType(NUnionDecl *union_decl) :
	union_decl(union_decl) { }

	virtual ~NUnionType()
	{
		delete union_decl;
	}

    virtual llvm::Type* getType(CodeGenContext& context);
};

class NBitFieldType : public NType {
public:
	int lineno = -1;
	char *file_name = NULL;
	unsigned bit_length;

    NBitFieldType(unsigned bit_length) :
	bit_length(bit_length) { }

	virtual ~NBitFieldType() {}

    virtual llvm::Type* getType(CodeGenContext& context);
};

class NTypedefDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	NType& type;
	Declarator& decl;

    NTypedefDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NTypedefDecl()
	{
		delete &type;
		delete &decl;
	}

    virtual CGValue codeGen(CodeGenContext& context);
};

class NBlock : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	StatementList statements;

	NBlock() { }

	virtual ~NBlock()
	{
		StatementList::const_iterator it;
		for (it = statements.begin(); it != statements.end(); it++) {
			delete *it;
		}
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NFunctionDecl : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	DeclSpecifier& func_specifier;
	Declarator& decl;
	NBlock *block;

	// specifiers: will be set by specifier
	SpecifierSet *specifiers;

	NFunctionDecl(DeclSpecifier& func_specifier, Declarator& decl,
				  NBlock *block) :
	func_specifier(func_specifier), decl(decl), block(block) { specifiers = new SpecifierSet(); }

	virtual ~NFunctionDecl()
	{
		delete &decl;

		DeclSpecifier::iterator di;
		for (di = func_specifier.begin();
			 di != func_specifier.end(); di++) {
			delete *di;
		}
		delete &func_specifier;

		delete block;
		delete specifiers;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

class NNameSpace : public NStatement {
public:
	int lineno = -1;
	char *file_name = NULL;
	std::string& name;
	NBlock *block;

	NNameSpace(std::string& name, NBlock *block) :
	name(name), block(block) { }

	virtual ~NNameSpace()
	{
		delete &name;
		if (block)
			delete block;
	}

	virtual CGValue codeGen(CodeGenContext& context);
};

#endif
