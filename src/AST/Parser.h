#ifndef _PARSER_H_
#define _PARSER_H_

#include "Node.h"
#include <stdio.h>
#include <string.h>
#include <map>
class CodeGenContext;
class Parser;

extern Parser *main_parser;
extern FILE *yyin;
extern int yyparse();
extern int yylex_destroy();

class Parser {
	NBlock *syntax_tree = NULL;
	vector<NStatement *> *extern_decls;

public:
	NBlock* getAST()
	{
		return syntax_tree;
	}

	void setAST(NBlock* tree)
	{
		syntax_tree = tree;
		return;
	}

	void generateAllDecl(CodeGenContext& context)
	{
		vector<NStatement *>::const_iterator decl_it;

		for (decl_it = extern_decls->begin();
			 decl_it != extern_decls->end(); decl_it++) {
			(*decl_it)->codeGen(context);
		}

		return;
	}

	void addDecl(NStatement *func)
	{
		extern_decls->push_back(func);
		return;
	}

	void startParse(FILE *fp)
	{
		main_parser = this;
		yyin = fp;
		yyparse();
		yylex_destroy();
		fclose(fp);

		return;
	}

	Parser()
	{
		syntax_tree = new NBlock();
		extern_decls = new vector<NStatement *>();
	}

	~Parser()
	{
		delete syntax_tree;

		vector<NStatement *>::const_iterator decl_it;
		for (decl_it = extern_decls->begin();
			 decl_it != extern_decls->end(); decl_it++) {
			free(*decl_it);
		}
		delete extern_decls;

		extern std::map<std::string, int> type_def;
		type_def.erase(type_def.begin(), type_def.end());
	}
};

#endif
