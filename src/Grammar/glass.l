
SP				[ \t\v\f]
NEWLINE			[\r\n]
PM				[+-]
DIGIT			[0-9]
HEX				[a-fA-F0-9]
OCT				[0-7]
LETTER			[a-zA-Z_]
ISUFFIX			(l|L|s|S|u|U)+
FSUFFIX			(l|L)

DIGITS			({DIGIT})+
SPS				({SP})+

%{
#include <stdio.h>
#include <string>
#include <map>
#include "AST/Node.h"
#include "Parser.hpp"
#include "AST/ASTErr.h"

#define SAVE_TOKEN()		(yylval.string = new std::string(yytext, yyleng))
#define TOKEN(t)			(yylval.token = t)
#define LINE_NUMBER_INC()	(current_line_number++)

int current_line_number = 1;
std::string* string_literal = NULL;
bool is_string = false;
extern CodeGenContext *global_context;
extern std::map<std::string, int> type_def;
extern char *current_file;

#define BUFFER_SIZE 1024
#define ARG_SIZE 10
void
setFile(char *text, int length)
{
	int i;
	char *file = NULL;
	char tmp_buf[BUFFER_SIZE];
	int args[ARG_SIZE] = { 0 };
	int arg_count = 0;

	for (i = 0; i < length; i++) {
		switch (text[i]) {
			case '#': continue;
			case ' ': continue;
			case '\t': continue;
			case '\v': continue;
			case '\f': continue;
			case '"':
				sscanf(&text[++i], "%s\"", tmp_buf);
				tmp_buf[strlen(tmp_buf) - 1] = '\0';
				file = strdup(tmp_buf);
				i += strlen(file) + 1;
				break;
			default:
				sscanf(&text[i], "%d", &args[arg_count]);
				sprintf(tmp_buf, "%d", args[arg_count]);
				i += strlen(tmp_buf) - 1;
				arg_count++;
				break;
		}
	}

	//printf("%d, %s, %d, %d, %d\n", args[0], file, args[1], args[2], args[3]);
	current_file = file;
	current_line_number = args[0];

	return;
}
%}

%option noyywrap
%start C_COMMENT CC_COMMENT STRING_LITERAL_STATE

%%
<INITIAL>#{SPS}{DIGITS}{SPS}\".*\"({SPS}{DIGITS})*{NEWLINE} {
	setFile(yytext, yyleng);
}

<INITIAL>"namespace"						return TOKEN(TNAMESPACE);
<INITIAL>"if"								return TOKEN(TIF);
<INITIAL>"else"							return TOKEN(TELSE);
<INITIAL>"for"								return TOKEN(TFOR);
<INITIAL>"while"							return TOKEN(TWHILE);
<INITIAL>"true"							return TOKEN(TTRUE);
<INITIAL>"false"							return TOKEN(TFALSE);
<INITIAL>"sizeof"							return TOKEN(TSIZEOF);
<INITIAL>"alignof"						return TOKEN(TALIGNOF);
<INITIAL>"typeof"							return TOKEN(TTYPEOF);
<INITIAL>"delegate"						return TOKEN(TDELEGATE);
<INITIAL>"struct"							return TOKEN(TSTRUCT);
<INITIAL>"extern"							return TOKEN(TEXTERN);
<INITIAL>"static"							return TOKEN(TSTATIC);
<INITIAL>"typedef"						return TOKEN(TTYPEDEF);
<INITIAL>"union"							return TOKEN(TUNION);
<INITIAL>"goto"							return TOKEN(TGOTO);
<INITIAL>"break"							return TOKEN(TBREAK);
<INITIAL>"continue"						return TOKEN(TCONTINUE);
<INITIAL>"return"							return TOKEN(TRETURN);

 /* Assignment */
<INITIAL>"="								return TOKEN(TASSIGN);
<INITIAL>"+="								return TOKEN(TAADD);
<INITIAL>"-="								return TOKEN(TASUB);
<INITIAL>"*="								return TOKEN(TAMUL);
<INITIAL>"/="								return TOKEN(TADIV);
<INITIAL>"%="								return TOKEN(TAMOD);
<INITIAL>"<<="								return TOKEN(TASHL);
<INITIAL>">>="								return TOKEN(TASHR);
<INITIAL>"&="								return TOKEN(TAAND);
<INITIAL>"^="								return TOKEN(TAXOR);
<INITIAL>"|="								return TOKEN(TAOR);

 /* Condition */
<INITIAL>"=="								return TOKEN(TCEQ);
<INITIAL>"!="								return TOKEN(TCNE);
<INITIAL>"<"								return TOKEN(TCLT);
<INITIAL>"<="								return TOKEN(TCLE);
<INITIAL>">"								return TOKEN(TCGT);
<INITIAL>">="								return TOKEN(TCGE);
<INITIAL>"?"								return TOKEN(TQUEM);

<INITIAL>"->"								return TOKEN(TARROW);

<INITIAL>"("								return TOKEN(TLPAREN);
<INITIAL>")"								return TOKEN(TRPAREN);
<INITIAL>"["								return TOKEN(TLBRAKT);
<INITIAL>"]"								return TOKEN(TRBRAKT);
<INITIAL>"{"								return TOKEN(TLBRACE);
<INITIAL>"}"								return TOKEN(TRBRACE);

<INITIAL>"&&"								return TOKEN(TLAND);
<INITIAL>"||"								return TOKEN(TLOR);

<INITIAL>"|"								return TOKEN(TOR);
<INITIAL>"^"								return TOKEN(TXOR);

<INITIAL>"..."								return TOKEN(TELLIPSIS);
<INITIAL>"::"								return TOKEN(TDCOLON);
<INITIAL>":"								return TOKEN(TCOLON);
<INITIAL>";"								return TOKEN(TSEMICOLON);
<INITIAL>"."								return TOKEN(TDOT);
<INITIAL>","								return TOKEN(TCOMMA);

 /* Unary */
<INITIAL>"&"								return TOKEN(TAND);
<INITIAL>"~"								return TOKEN(TNOT);
<INITIAL>"!"								return TOKEN(TLNOT);
<INITIAL>"++"								return TOKEN(TINC);
<INITIAL>"--"								return TOKEN(TDEC);

 /* Binary */
<INITIAL>"+"								return TOKEN(TADD);
<INITIAL>"-"								return TOKEN(TSUB);
<INITIAL>"*"								return TOKEN(TMUL);
<INITIAL>"/"								return TOKEN(TDIV);
<INITIAL>"%"								return TOKEN(TMOD);

<INITIAL>">>"								return TOKEN(TSHR);
<INITIAL>"<<"								return TOKEN(TSHL);

<INITIAL>{NEWLINE}						LINE_NUMBER_INC();
<INITIAL>{SP}								/* Blank */;

 /* Constants */
<INITIAL>{LETTER}({LETTER}|{DIGIT})* {
	SAVE_TOKEN();
	if (type_def.find(*yylval.string) != type_def.end()) {
		return TTYPE_NAME;
	}
	return TIDENTIFIER;
}
<INITIAL>{DIGIT}+\.{DIGIT}+{FSUFFIX}? {
	SAVE_TOKEN();
	return TDOUBLE;
}
<INITIAL>"0"{OCT}+{ISUFFIX}? { // oct
	SAVE_TOKEN();
	return TINTEGER;
}
<INITIAL>"0"[xX]{HEX}+{ISUFFIX}? { // hex
	SAVE_TOKEN();
	return TINTEGER;
}
<INITIAL>{DIGIT}+{ISUFFIX}? { // dec
	SAVE_TOKEN();
	return TINTEGER;
}

 /* Comments */
<INITIAL>"/*"     							BEGIN C_COMMENT;
<INITIAL>"//"     							BEGIN CC_COMMENT;

<C_COMMENT>{NEWLINE} {
	LINE_NUMBER_INC();
}
<C_COMMENT>"*/"							BEGIN INITIAL;
<C_COMMENT><<EOF>> {
	printf("EOF in comment\n");
	std::exit(1);
}
<C_COMMENT>.								;
<CC_COMMENT>{NEWLINE} {
	LINE_NUMBER_INC();
	BEGIN INITIAL;
}
<CC_COMMENT><<EOF>>	BEGIN INITIAL;
<CC_COMMENT>.			;

 /* String */
<INITIAL>\" {
	string_literal = new std::string("", 0);
	is_string = true;
    BEGIN STRING_LITERAL_STATE;
}
<INITIAL>\' {
	string_literal = new std::string("", 0);
	is_string = false;
    BEGIN STRING_LITERAL_STATE;
}
<STRING_LITERAL_STATE>\" {
	if (is_string) {
		yylval.string = string_literal;
		string_literal = NULL;
		BEGIN INITIAL;
		return TSTRING;
	} else {
		*string_literal += '"';
	}
}
<STRING_LITERAL_STATE>\' {
	if (is_string) {
		*string_literal += '\'';
	} else {
		if (string_literal->size() > 1) {
			ErrorMessage::tmpError(ASTERR_Too_Much_Characters());
		}
		yylval.character = string_literal->c_str()[0];
		string_literal = NULL;
		BEGIN INITIAL;
		return TCHAR;
	}
}
<STRING_LITERAL_STATE>\\{OCT}{1,3} {
	int letter;
	sscanf(&yytext[1], "%o", &letter);
    *string_literal += letter;
}
<STRING_LITERAL_STATE>\\[xX]{HEX}{1,2} {
	int letter;
	sscanf(&yytext[2], "%x", &letter);
    *string_literal += letter;
}
<STRING_LITERAL_STATE>{NEWLINE}        {
	*string_literal += yytext[0];
    LINE_NUMBER_INC();
}
<STRING_LITERAL_STATE>\\\"      *string_literal += '"';
<STRING_LITERAL_STATE>\\'       *string_literal += '\'';
<STRING_LITERAL_STATE>\\n       *string_literal += '\n';
<STRING_LITERAL_STATE>\\t       *string_literal += '\t';
<STRING_LITERAL_STATE>\\\\      *string_literal += '\\';
<STRING_LITERAL_STATE><<EOF>>   {
	printf("EOF in string literal\n");
	yyterminate();
}
<STRING_LITERAL_STATE>.         {
    *string_literal += yytext[0];
}

. {
	printf("Unknown token\n");
	yyterminate();
}
%%
