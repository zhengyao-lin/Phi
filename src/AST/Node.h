#ifndef _NODE_H_
#define _NODE_H_

#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/GlobalValue.h>

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
	virtual ~Node() {}
	virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};
class NStatement : public Node {
public:
	int lineno = -1;
	virtual ~NStatement() {}
};
class NExpression : public NStatement {
public:
	int lineno = -1;
	virtual ~NExpression() {}
};

class NIdentifier : public NExpression {
public:
	int lineno = -1;
	std::string& name;

	NIdentifier(std::string& name) :
	name(name) { }

	virtual ~NIdentifier()
	{
		delete &name;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
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
	virtual ~Declarator() {}
	virtual DeclInfo *getDeclInfo(CodeGenContext& context, llvm::Type *base_type)
	{
		return new DeclInfo(base_type, NULL);
	}
};

class IdentifierDeclarator : public Declarator {
public:
	int lineno = -1;
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
	virtual llvm::Type* getType(CodeGenContext& context);
	virtual ~NType() {}
};

class NDerivedType : public NType {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context)
	{
		return NULL;
	}

	virtual ~NVoid() {}
};
class NInteger : public NExpression {
public:
	int lineno = -1;
	std::string& value;

	NInteger(std::string& value) :
	value(value) { }

	virtual ~NInteger()
	{
		delete &value;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NChar : public NExpression {
public:
	int lineno = -1;
	char value;

	NChar(char value) :
	value(value) { }

	virtual ~NChar()
	{ }

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NBoolean : public NExpression {
public:
	int lineno = -1;
	bool value;

	NBoolean(bool value) :
	value(value) { }

	virtual ~NBoolean() {}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NDouble : public NExpression {
public:
	int lineno = -1;
	double value;

	NDouble(double value) :
	value(value) { }

	virtual ~NDouble() {}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NString : public NExpression {
public:
	int lineno = -1;
	std::string value;

	NString(const std::string& value) :
	value(value) { }

	virtual ~NString() {}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
// Primary Expression End

// Postfix Expression
class NMethodCall : public NExpression {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NFieldExpr : public NExpression {
public:
	int lineno = -1;
	NExpression& operand;
	NIdentifier& field_name;

	NFieldExpr(NExpression& operand, NIdentifier& field_name) :
	operand(operand), field_name(field_name) { }

	virtual ~NFieldExpr()
	{
		delete &operand;
		delete &field_name;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
class NArrayExpr : public NExpression {
public:
	int lineno = -1;
	NExpression& operand;
	NExpression& index;

	NArrayExpr(NExpression& operand, NExpression& index) :
	operand(operand), index(index) { }

	virtual ~NArrayExpr()
	{
		delete &operand;
		delete &index;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};
// Postfix Expression End

// Binary Expression
class NBinaryExpr : public NExpression {
public:
	int lineno = -1;
	int op;
	NExpression& lval;
	NExpression& rval;

	NBinaryExpr(NExpression& lval, int op, NExpression& rval) :
	lval(lval), rval(rval), op(op) { }

	virtual ~NBinaryExpr()
	{
		delete &lval;
		delete &rval;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

// Prefix Expression
class NPrefixExpr : public NExpression {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NIncDecExpr : public NExpression {
public:
	int lineno = -1;
	int op;
	NExpression& operand;

	NIncDecExpr(NExpression& operand, int op) :
	operand(operand), op(op) { }

	virtual ~NIncDecExpr()
	{
		delete &operand;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NAssignmentExpr : public NExpression {
public:
	int lineno = -1;
	NExpression& lval;
	NExpression& rval;

	NAssignmentExpr(NExpression& lval, NExpression& rval) :
	lval(lval), rval(rval) { }

	static llvm::Value *doAssignCast(CodeGenContext& context, llvm::Value *value,
									  llvm::Type *variable_type, llvm::Value *variable, int lineno);

	virtual ~NAssignmentExpr()
	{
		delete &lval;
		delete &rval;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NTypeof : public NType {
public:
	int lineno = -1;
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
	NExpression& expression;

	NReturnStatement(NExpression& expression) :
	expression(expression) { }

	virtual ~NReturnStatement()
	{
		delete &expression;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NIfStatement : public NStatement {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NParamDecl : public NStatement {
public:
	int lineno = -1;
	NType& type;
	Declarator& decl;

	NParamDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NParamDecl()
	{
		delete &type;
		delete &decl;
	}

	// virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NLabelStatement : public NStatement {
public:
	std::string label_name;
	NStatement& statement;

	NLabelStatement(std::string label_name, NStatement& statement) :
	label_name(label_name), statement(statement) { }

	virtual ~NLabelStatement()
	{
		delete &statement;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NGotoStatement : public NStatement {
public:
	std::string label_name;

	NGotoStatement(std::string label_name) :
	label_name(label_name) { }

	virtual ~NGotoStatement() {}
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NDelegateDecl : public NStatement {
public:
	int lineno = -1;
	NType& type;
	Declarator& decl;

	NDelegateDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NDelegateDecl()
	{
		delete &type;
		delete &decl;
	}

    virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NVariableDecl : public NStatement {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NStructDecl : public NStatement {
public:
	int lineno = -1;
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

    virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NUnionDecl : public NStatement {
public:
	int lineno = -1;
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

    virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NStructType : public NType {
public:
	int lineno = -1;
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
	unsigned bit_length;

    NBitFieldType(unsigned bit_length) :
	bit_length(bit_length) { }

	virtual ~NBitFieldType() {}

    virtual llvm::Type* getType(CodeGenContext& context);
};

class NTypedefDecl : public NStatement {
public:
	int lineno = -1;
	NType& type;
	Declarator& decl;

    NTypedefDecl(NType& type, Declarator& decl) :
	type(type), decl(decl) { }

	virtual ~NTypedefDecl()
	{
		delete &type;
		delete &decl;
	}

    virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NBlock : public NStatement {
public:
	int lineno = -1;
	StatementList statements;

	NBlock() { }

	virtual ~NBlock()
	{
		StatementList::const_iterator it;
		for (it = statements.begin(); it != statements.end(); it++) {
			delete *it;
		}
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NFunctionDecl : public NStatement {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NNameSpace : public NStatement {
public:
	int lineno = -1;
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

	virtual llvm::Value* codeGen(CodeGenContext& context);
};

#endif
