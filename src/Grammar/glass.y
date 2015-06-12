%{
	#include "AST/Node.h"
	#include "AST/ASTErr.h"
	#include "AST/Parser.h"
    #include <cstdio>
    #include <cstdlib>
	#include <cstring>
	#include <map>
	#define SETLINE(p) ((p)->lineno = current_line_number, (p)->file_name = current_file)

	extern Parser *main_parser;
	extern CodeGenContext *global_context;
	std::map<std::string, int> type_def;
	char *current_file = "<no_name>";

	void
	ASTERR_Undefined_Syntax_Error(const char *token) {
		error_messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
								  "Undefined syntax error (near \"$(token)\")", token));
		return;
	}

	void
	yyerror(const char *msg) {
		ASTERR_Undefined_Syntax_Error(yytext);
		ASTERR_setLineNumber();
		ASTERR_showAllMsgAndExit1();
	}
	int getAssignToBinary(int token);
%}

%union {
	Node *node;
	NBlock *block;
	NExpression *expression;
	NStatement *statement;
	NIdentifier *identifier;
	NType *type;
	NVariableDecl *variable_declaration;
	std::vector<NVariableDecl*> *variable_list;
	std::vector<NParamDecl*> *param_list;
	std::vector<NExpression*> *expression_list;
	std::vector<NExpression*> *array_dim;
	NSpecifier *specifier;
	std::vector<NSpecifier*> *declaration_specifier;
	Declarator *declarator;
	DeclaratorList *declarator_list;
	NParamDecl *param_declaration;
	std::string *string;
	char character;
	int token;
	int dim;
}

%token <string> TIDENTIFIER TTYPE_NAME TINTEGER TDOUBLE TSTRING TTRUE TFALSE
%token <character> TCHAR
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TASSIGN
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE
				TAND TNOT TSIZEOF TALIGNOF TTYPEOF TARROW
			    TELLIPSIS TCOLON TSEMICOLON TCOMMA TDOT
				TLBRAKT TRBRAKT TLAND TLOR TOR TXOR TIF TELSE TWHILE
				TLNOT TNAMESPACE TDCOLON TINC TDEC TQUEM TFOR
%token <token> TADD TSUB TMUL TDIV TMOD TSHR TSHL
				TAADD TASUB TAMUL TADIV TAMOD TASHR TASHL TAAND TAOR TAXOR
%token <token> TRETURN TEXTERN TDELEGATE TSTRUCT TSTATIC
				TTYPEDEF TUNION TGOTO TBREAK TCONTINUE

%type <identifier> identifier type_name namespace_header
%type <expression> numeric string_literal expression
					assignment_expression unary_expression primary_expression
					postfix_expression conditional_expression additive_expression
					multiplicative_expression cast_expression
					shift_expression initializer_opt
%type <expression> logical_or_expression logical_and_expression inclusive_or_expression
					exclusive_or_expression and_expression equality_expression
					relational_expression

%type <type> type_specifier identifier_type bitfield_type struct_type union_type ex_type_specifier
%type <variable_list> fields_declaration
%type <param_list> function_arguments
%type <expression_list> expression_list
%type <array_dim> array_dim
%type <specifier> storage_specifier class_specifier prefix_specifier
%type <declaration_specifier> declaration_specifier
%type <declarator> init_declarator declarator direct_declarator
%type <declarator_list> declarator_list
%type <param_declaration> param_declaration
%type <block> statement_list block in_namespace_declaration_list
%type <statement> statement external_declaration declarations variable_declaration field_variable_declaration
				   function_definition ret_statement delegate_declaration
				   struct_declaration union_declaration typedef_declaration
				   selection_statement labeled_statement jump_statement
				   namespace_declaration in_namespace_declaration
				   expression_statement iteration_statement //struct_or_union_declaration
%type <token> assign_op unary_op self_assign_op
%type <dim>	ptr_dim

%start compile_unit

%%

compile_unit
	: external_declaration
	{
		main_parser->getAST()->statements.push_back($1);
	}
	| compile_unit external_declaration
	{
		main_parser->getAST()->statements.push_back($2);
	}
	;

namespace_header
	: TNAMESPACE identifier TLBRACE
	{
		$$ = $2;
		SETLINE($$);
	}
	;

namespace_declaration
	: namespace_header in_namespace_declaration_list TRBRACE
	{
		$$ = new NNameSpace(*new std::string($1->name), $2);
		delete $1;
	}
	| namespace_header TRBRACE
	{
		$$ = new NNameSpace(*new std::string($1->name), NULL);
		delete $1;
	}
	;

in_namespace_declaration_list
	: in_namespace_declaration
	{
		$$ = new NBlock();
		$$->statements.push_back($1);
	}
	| in_namespace_declaration_list in_namespace_declaration
	{
		$1->statements.push_back($2);
		$$ = $1;
	}
	;

in_namespace_declaration
	: function_definition
	| namespace_declaration
	| declarations TSEMICOLON
	;

external_declaration
	: function_definition
	{
		/*NFunctionDecl *func_decl = new NFunctionDecl(((NFunctionDecl *)$1)->func_specifier,
													 ((NFunctionDecl *)$1)->id,
													 ((NFunctionDecl *)$1)->arguments, NULL,
													 ((NFunctionDecl *)$1)->has_vargs);
		SETLINE(func_decl);
		main_parser->addDecl(func_decl);*/

		$$ = $1;
	}
	| namespace_declaration
	{
		//main_parser->addDecl($1);
		$$ = $1;
	}
	| declarations TSEMICOLON
	{
		//main_parser->addDecl($1);
		$$ = $1;
	}
	;

declarations
	: variable_declaration
	| delegate_declaration
	| struct_declaration
	| union_declaration
	| typedef_declaration
	;

storage_specifier
	: TEXTERN
	{
		$$ = new NExternSpecifier();
	}
	| TSTATIC
	{
		$$ = new NStaticSpecifier();
	}
	;

class_specifier
	: type_specifier
	{
		$$ = new NTypeSpecifier(*$1);
	}
	;

prefix_specifier
	: storage_specifier
	| class_specifier
	;

declaration_specifier
	: prefix_specifier
	{
		$$ = new DeclSpecifier();
		$$->push_back($1);
	}
	| declaration_specifier prefix_specifier
	{
		$1->push_back($2);
		$$ = $1;
	}
	;

ellipsis_token:TCOMMA TELLIPSIS;
function_definition
	: declaration_specifier declarator block
	{
		$$ = new NFunctionDecl(*$1, *$2, $3);
		SETLINE($$);
	}
	;

param_declaration
	: type_specifier declarator
	{
		$$ = new NParamDecl(*$1, *$2);
		SETLINE($$);
	}
	| type_specifier
	{
		$$ = new NParamDecl(*$1, *new Declarator());
		SETLINE($$);
	}
	;

function_arguments
	: param_declaration
	{
		$$ = new ParamList();
		$$->push_back($<param_declaration>1);
	}
	| function_arguments TCOMMA param_declaration
	{
		$1->push_back($<param_declaration>3);
	}
	;

type_specifier
	: identifier_type
	{
		SETLINE($1);
	}
	| struct_type
	{
		SETLINE($1);
	}
	| union_type
	{
		SETLINE($1);
	}
	| bitfield_type
	{
		SETLINE($1);
	}
	| TTYPEOF unary_expression
	{
		$$ = new NTypeof(*$2);
		SETLINE($$);
	}
	;

initializer_opt
	: /* Blank */
	{
		$$ = NULL;
	}
	| TASSIGN assignment_expression
	{
		$$ = $2;
	}
	;

direct_declarator
	: identifier
	{
		$$ = new IdentifierDeclarator(*$1);
	}
	| TLPAREN declarator TRPAREN
	{
		$$ = $2;
	}
	| direct_declarator array_dim
	{
		$$ = new ArrayDeclarator(*$1, *$2);
	}
	| direct_declarator TLPAREN function_arguments TRPAREN
	{
		$$ = new ParamDeclarator(*$1, *$3, false);
	}
	| direct_declarator TLPAREN TRPAREN
	{
		$$ = new ParamDeclarator(*$1, *new ParamList(), false);
	}
	| direct_declarator TLPAREN function_arguments ellipsis_token TRPAREN
	{
		$$ = new ParamDeclarator(*$1, *$3, true);
	}
	;

declarator
	: direct_declarator
	| ptr_dim direct_declarator
	{
		$$ = new PointerDeclarator($1, *$2);
	}
	;

init_declarator
	: declarator initializer_opt
	{
		$$ = new InitDeclarator(*$1, $2);
	}
	;

declarator_list
	: init_declarator
	{
		$$ = new DeclaratorList();
		$$->push_back($1);
	}
	| declarator_list TCOMMA init_declarator
	{
		$1->push_back($3);
		$$ = $1;
	}
	;

variable_declaration
	: declaration_specifier declarator_list
	{
		$$ = new NVariableDecl(*$1, $2);
		SETLINE($$);
	}
	;

delegate_declaration
	: TDELEGATE type_specifier declarator
	{
		DeclInfo *decl_info = $3->getDeclInfo(*global_context, NULL);
		type_def[decl_info->id->name] = 0;

		$$ = new NDelegateDecl(*$2, *$3);
		SETLINE($$);

		delete decl_info;
	}
	| TDELEGATE TSTATIC type_specifier declarator
	{
		ASTERR_Static_Specifier_In_Delegate();
		ASTERR_setLineNumber();
		ASTERR_showAllMsg();

		DeclInfo *decl_info = $4->getDeclInfo(*global_context, NULL);
		type_def[decl_info->id->name] = 0;

		$$ = new NDelegateDecl(*$3, *$4);
		SETLINE($$);

		delete decl_info;
	}
	;

field_variable_declaration
	: variable_declaration
	| struct_declaration
	{
		DeclSpecifier *decl_spec = new DeclSpecifier();
		decl_spec->push_back(new NTypeSpecifier(*new NStructType((NStructDecl*)$1)));

		DeclaratorList *decl_list = new DeclaratorList();
		decl_list->push_back(new IdentifierDeclarator(*new NIdentifier(*new std::string("."))));

		$$ = new NVariableDecl(*decl_spec, decl_list);
	}
	| union_declaration
	{
		DeclSpecifier *decl_spec = new DeclSpecifier();
		decl_spec->push_back(new NTypeSpecifier(*new NUnionType((NUnionDecl*)$1)));

		DeclaratorList *decl_list = new DeclaratorList();
		decl_list->push_back(new IdentifierDeclarator(*new NIdentifier(*new std::string("."))));

		$$ = new NVariableDecl(*decl_spec, decl_list);
	}
	;

fields_declaration
	: /* Blank */
	{
		$$ = new VariableList();
	}
	| field_variable_declaration TSEMICOLON
	{
		$$ = new VariableList();
		$$->push_back($<variable_declaration>1);
	}
	| fields_declaration field_variable_declaration TSEMICOLON
	{
		$1->push_back($<variable_declaration>2);
	}
	;

 /*struct_or_union_declaration
	: struct_declaration
	| union_declaration
	;*/

struct_declaration
	: TSTRUCT identifier TLBRACE
	  fields_declaration
	  TRBRACE
	{
		$$ = new NStructDecl(*$2, $4);
		SETLINE($$);
	}
	| TSTRUCT identifier
	{
		$$ = new NStructDecl(*$2, NULL);
		SETLINE($$);
	}
	| TSTRUCT TLBRACE
	  fields_declaration
	  TRBRACE
	{
		$$ = new NStructDecl(*new NIdentifier(*new std::string(".")),
							 $3);
		SETLINE($$);
	}
	;

union_declaration
	: TUNION identifier TLBRACE
	  fields_declaration
	  TRBRACE
	{
		$$ = new NUnionDecl(*$2, $4);
		SETLINE($$);
	}
	| TUNION identifier
	{
		$$ = new NUnionDecl(*$2, NULL);
		SETLINE($$);
	}
	| TUNION TLBRACE
	  fields_declaration
	  TRBRACE
	{
		$$ = new NUnionDecl(*new NIdentifier(*new std::string(".")),
							$3);
		SETLINE($$);
	}
	;

typedef_declaration
	: TTYPEDEF type_specifier declarator
	{
		DeclInfo *decl_info = $3->getDeclInfo(*global_context, NULL);
		type_def[decl_info->id->name] = 0;

		$$ = new NTypedefDecl(*$2, *$3);
		SETLINE($$);

		delete decl_info;
	}
	;

ptr_dim
	: TMUL
	{
		$$ = 1;
	}
	| TMUL ptr_dim
	{
		$$ = $2 + 1;
	}
	;

array_dim
	: TLBRAKT assignment_expression TRBRAKT
	{
		$$ = new ArrayDim();
		$$->push_back($2);
	}
	| TLBRAKT TRBRAKT
	{
		$$ = new ArrayDim();
		$$->push_back(NULL);
	}
	| TLBRAKT assignment_expression TRBRAKT array_dim
	{
		$4->push_back($2);
		$$ = $4;
	}
	| TLBRAKT TRBRAKT array_dim
	{
		$3->push_back(NULL);
		$$ = $3;
	}
	;

identifier_type
	: type_name
	{
		$$ = new NIdentifierType(*$1);
		SETLINE($$);
	}
	;

bitfield_type
	: type_name TCOLON TINTEGER
	{
		if ($1->name.compare("int")) {
			ASTERR_Bit_Field_With_Non_Int_Type();
			ASTERR_setLineNumber();
			ASTERR_showAllMsg();
		}

		$$ = new NBitFieldType((unsigned)atol($3->c_str()));
		SETLINE($$);

		delete $1;
		delete $3;
	}
	;

struct_type
	: struct_declaration
	{
		$$ = new NStructType((NStructDecl*)$1);
		SETLINE($$);
	}
	/*| TSTRUCT identifier
	{
		$$ = new NStructType($2);
		SETLINE($$);
	}*/
	;

union_type
	: union_declaration
	{
		$$ = new NUnionType((NUnionDecl*)$1);
		SETLINE($$);
	}
	;

statement_list
	: statement
	{
		$$ = new NBlock();
		$$->statements.push_back($1);
	}
	| statement_list statement
	{
		$1->statements.push_back($2);
		$$ = $1;
	}
	;

statement
	: function_definition
	| jump_statement TSEMICOLON
	| expression_statement
	| declarations TSEMICOLON
	| block
	{
		$$ = $<statement>1;
		SETLINE($$);
	}
	| selection_statement
	| iteration_statement
	| labeled_statement
	;

iteration_statement
	: TWHILE TLPAREN expression TRPAREN statement
	{
		$$ = new NWhileStatement(*$3, $5);
		SETLINE($$);
	}
	| TFOR TLPAREN expression_statement expression_statement expression TRPAREN statement
	{
		$$ = new NForStatement(*(NExpression*)$3, *(NExpression*)$4, *$5, $7);
		SETLINE($$);
	}
	| TFOR TLPAREN expression_statement expression_statement TRPAREN statement
	{
		$$ = new NForStatement(*(NExpression*)$3, *(NExpression*)$4, *new NVoid(), $6);
		SETLINE($$);
	}
	;

expression_statement
	: TSEMICOLON
	{
		$$ = new NVoid();
		SETLINE($$);
	}
	| expression TSEMICOLON
	{
		$$ = $<statement>1;
		SETLINE($$);
	}
	;

selection_statement
	: TIF TLPAREN expression TRPAREN statement
	{
		$$ = new NIfStatement(*$3, $5, NULL);
		SETLINE($$);
	}
	| TIF TLPAREN expression TRPAREN statement TELSE statement
	{
		$$ = new NIfStatement(*$3, $5, $7);
		SETLINE($$);
	}
	;

labeled_statement
	: identifier TCOLON statement
	{
		$$ = new NLabelStatement($1->name, *$3);
		delete $1;
		SETLINE($$);
	}
	;

jump_statement
	: TGOTO identifier
	{
		$$ = new NGotoStatement($2->name);
		delete $2;
		SETLINE($$);
	}
	| TBREAK
	{
		$$ = new NJumpStatement(false);
		SETLINE($$);
	}
	| TCONTINUE
	{
		$$ = new NJumpStatement(true);
		SETLINE($$);
	}
	| ret_statement
	;

block
	: TLBRACE statement_list TRBRACE
	{
		$$ = $2;
		SETLINE($$);
	}
	| TLBRACE TRBRACE
	{
		$$ = new NBlock();
		SETLINE($$);
	}
	;

ret_statement
	: TRETURN expression
	{
		$$ = new NReturnStatement(*$2);
		SETLINE($$);
	}
	| TRETURN
	{
		$$ = new NReturnStatement(*(new NVoid()));
		SETLINE($$);
	}
	;

identifier
	: TIDENTIFIER
	{
		$$ = new NIdentifier(*$1);
		SETLINE($$);
	}
	| identifier TDCOLON TIDENTIFIER
	{
		$$ = new NIdentifier(*new std::string($1->name + "$" + *$3));
		SETLINE($$);

		delete $1;
		delete $3;
	}
	;

type_name
	: TTYPE_NAME
	{
		$$ = new NIdentifier(*$1);
		SETLINE($$);
	}
	| identifier TDCOLON TTYPE_NAME
	{
		$$ = new NIdentifier(*new std::string($1->name + "$" + *$3));
		SETLINE($$);

		delete $1;
		delete $3;
	}
	;

numeric
	: TINTEGER
	{
		$$ = new NInteger(*$1);
		SETLINE($$);
		//delete $1;
	}
	| TDOUBLE
	{
		$$ = new NDouble(atof($1->c_str()));
		SETLINE($$);
		delete $1;
	}
	| TCHAR
	{
		$$ = new NChar($1);
		SETLINE($$);
	}
	| TTRUE
	{
		$$ = new NBoolean(true);
		SETLINE($$);
	}
	| TFALSE
	{
		$$ = new NBoolean(false);
		SETLINE($$);
	}
	;

string_literal
	: TSTRING
	{
		$$ = new NString(*$1);
		SETLINE($$);
		delete $1;
	}
	;

primary_expression
	: identifier
	{
		$$ = $<expression>1;
		SETLINE($$);
	}
	| numeric
	| string_literal
	| TLPAREN expression TRPAREN
	{
		$$ = $2;
		SETLINE($$);
	}
	;

postfix_expression
	: primary_expression
	| postfix_expression TLPAREN TRPAREN
	{
		$$ = new NMethodCall(*$1, *new ExpressionList());
		SETLINE($$);
	}
	| postfix_expression TLPAREN expression_list TRPAREN
	{
		$$ = new NMethodCall(*$1, *$3);
		SETLINE($$);
	}
	| postfix_expression TINC
	{
		$$ = new NIncDecExpr(*$1, $2);
		SETLINE($$);
	}
	| postfix_expression TDEC
	{
		$$ = new NIncDecExpr(*$1, $2);
		SETLINE($$);
	}
	| postfix_expression TLBRAKT expression TRBRAKT
	{
		$$ = new NArrayExpr(*$1, *$3);
		SETLINE($$);
	}
	| postfix_expression TDOT identifier
	{
		$$ = new NFieldExpr(*$1, *$3);
		SETLINE($$);
	}
	| postfix_expression TARROW identifier
	{
		$$ = new NFieldExpr(*new NPrefixExpr(TMUL, *$1), // { expr->identifier } equals { (*expr).identifier }
							*$3);
		SETLINE($$);
	}
	;

unary_op
	: TSUB
	| TADD
	| TMUL
	| TAND
	| TNOT
	| TLNOT
	| TINC
	| TDEC
	| TDCOLON
	;

ex_type_specifier
	: type_specifier
	| type_specifier ptr_dim
	{
		$$ = new NDerivedType(*$1, $2);
		SETLINE($$);
	}
	;

unary_expression
	: postfix_expression
	| unary_op unary_expression
	{
		$$ = new NPrefixExpr($1, *$2);
		SETLINE($$);
	}
	| TSIZEOF TLPAREN ex_type_specifier TRPAREN
	{
		$$ = new NPrefixExpr($1, *$3);
		SETLINE($$);
	}
	| TALIGNOF TLPAREN ex_type_specifier TRPAREN
	{
		$$ = new NPrefixExpr($1, *$3);
		SETLINE($$);
	}
	;

cast_expression
	: unary_expression
	| TLPAREN ex_type_specifier TRPAREN cast_expression
	{
		$$ = new NPrefixExpr(*$2, *$4);
		SETLINE($$);
	}
	;

multiplicative_expression
	: cast_expression
	| multiplicative_expression TMUL cast_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| multiplicative_expression TDIV cast_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| multiplicative_expression TMOD cast_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

additive_expression
	: multiplicative_expression
	| additive_expression TADD multiplicative_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| additive_expression TSUB multiplicative_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

shift_expression
	: additive_expression
	| shift_expression TSHL additive_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| shift_expression TSHR additive_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

relational_expression
	: shift_expression
	| relational_expression TCLT shift_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| relational_expression TCGT shift_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| relational_expression TCLE shift_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| relational_expression TCGE shift_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

equality_expression
	: relational_expression
	| equality_expression TCEQ relational_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	| equality_expression TCNE relational_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

and_expression
	: equality_expression
	| and_expression TAND equality_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression TXOR and_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression TOR exclusive_or_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression TLAND inclusive_or_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression TLOR logical_and_expression
	{
		$$ = new NBinaryExpr(*$1, $2, *$3);
		SETLINE($$);
	}
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression TQUEM expression TCOLON conditional_expression
	{
		$$ = new NCondExpr(*$1, *$3, *$5);
		SETLINE($$);
	}
	;

assign_op
	: TASSIGN
	;

self_assign_op
	: TAADD
	| TASUB
	| TAMUL
	| TADIV
	| TAMOD
	| TASHR
	| TASHL
	| TAAND
	| TAOR
	| TAXOR
	;

assignment_expression
	: conditional_expression
	| unary_expression assign_op assignment_expression
	{
		$$ = new NAssignmentExpr(*$1, *$3);
		SETLINE($$);
	}
	| unary_expression self_assign_op assignment_expression
	{
		$$ = new NAssignmentExpr(*$1, *new NBinaryExpr(*$1, getAssignToBinary($2), *$3, true));
		SETLINE($$);
	}
	;

expression
	: assignment_expression
	{
		SETLINE($$);
	}
	| expression TCOMMA assignment_expression
	{
		$$ = new NCompoundExpr(*$1, *$3);
		SETLINE($$);
	}
	;

expression_list
	: assignment_expression
	{
		$$ = new ExpressionList();
		$$->push_back($1);
	}
	| expression_list TCOMMA assignment_expression
	{
		$1->push_back($3);
	}
	;
%%
int getAssignToBinary(int token)
{
	switch(token) {
		case TAADD:
			return TADD;
		case TASUB:
			return TSUB;
		case TAMUL:
			return TMUL;
		case TADIV:
			return TDIV;
		case TAMOD:
			return TMOD;
		case TASHR:
			return TSHR;
		case TASHL:
			return TSHL;
		case TAAND:
			return TAND;
		case TAOR:
			return TOR;
		case TAXOR:
			return TXOR;
	}

	return -1;
}
