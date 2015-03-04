#ifndef _EMCORE_H_
#define _EMCORE_H_

#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <string>
#include "stdarg.h"

#define NONE         "\033[m"
#define BOLD         "\033[1m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"
#define DEFAULT      "\033[0m"
#define NEXT_LINE    "\n"
#define CLOSE_COLOR  "\033[0m\n"

using namespace std;

class ErrorInfo {
public:
	enum ActionFlag {
		NoAct,
		Exit0,
		Exit1,
		Abort
	};
	enum Prefix {
		None,
		Note,
		Warning,
		Error
	};

private:
	string MSG;
	ActionFlag AF;

	string setMessageStyle(string message, Prefix prefix)
	{
		switch (prefix) {
			case None:
				break;
			case Note:
				message = LIGHT_BLUE + string("Note: ") + NONE + message;
				break;
			case Warning:
				message = YELLOW + string("Warning: ") + NONE + message;
				break;
			case Error:
				message = LIGHT_RED + string("Error: ") + NONE + message;
				break;
		}

		return message;
	}

	string setMessageStyle(string message, Prefix prefix, bool is_bold)
	{
		switch (prefix) {
			case None:
				message = (is_bold ? BOLD : NONE) + message + NONE;
				break;
			case Note:
				message = LIGHT_BLUE + string("Note: ") + NONE + (is_bold ? BOLD : NONE) + message + NONE;
				break;
			case Warning:
				message = YELLOW + string("Warning: ") + NONE + (is_bold ? BOLD : NONE) + message + NONE;
				break;
			case Error:
				message = LIGHT_RED + string("Error: ") + NONE + (is_bold ? BOLD : NONE) + message + NONE;
				break;
		}

		return message;
	}

	string createFormatMessage(string message, va_list args)
	{
		int i, length;

		for (i = 0; i < message.size() && message.c_str()[i] != '\0'; i++)
		{
			if (message.c_str()[i] != '$') {
				continue;
			} else if (message.c_str()[i + 1] != '(') {
				continue;
			}

			for (length = 1; message.c_str()[i + length] != ')'; length++);
			message.replace(i, length + 1, va_arg(args, char *));
			i += length;
		}

		return message;
	}

public:
	ErrorInfo(string message, ...)
	: AF(NoAct)
	{
		va_list args;

		va_start(args, message);
		MSG = createFormatMessage(message, args);
		va_end(args);

		return;
	}

	ErrorInfo(ActionFlag action, string message, ...)
	: AF(action)
	{
		va_list args;

		va_start(args, message);
		MSG = createFormatMessage(message, args);
		va_end(args);

		return;
	}

	ErrorInfo(Prefix prefix, string message, ...)
	: AF(NoAct)
	{
		va_list args;

		va_start(args, message);
		MSG = createFormatMessage(message, args);
		va_end(args);

		return;
	}

	ErrorInfo(Prefix prefix, bool is_bold, string message, ...)
	: AF(NoAct)
	{
		va_list args;

		va_start(args, message);
		MSG = setMessageStyle(createFormatMessage(message, args),
							  prefix, is_bold);
		va_end(args);

		return;
	}

	ErrorInfo(Prefix prefix, ActionFlag action, string message, ...)
	: AF(action)
	{
		va_list args;

		va_start(args, message);
		MSG = setMessageStyle(createFormatMessage(message, args),
							  prefix);
		va_end(args);

		return;
	}

	ErrorInfo(Prefix prefix, bool is_bold, ActionFlag action, string message, ...)
	: AF(action)
	{
		va_list args;

		va_start(args, message);
		MSG = setMessageStyle(createFormatMessage(message, args),
							  prefix, is_bold);
		va_end(args);

		return;
	}

	ActionFlag getActionFlag()
	{
		return AF;
	}

	void setActionFlag(ActionFlag action)
	{
		AF = action;
		return;
	}

	void doPrint(ostream& strm)
	{
		strm << MSG << endl;
		switch (AF) {
			case NoAct:
				break;
			case Exit0:
				exit(0);
				break;
			case Exit1:
				exit(1);
				break;
			case Abort:
				abort();
				break;
		}

		return;
	}

	void doPrint(ostream& strm, ActionFlag action)
	{
		strm << MSG << endl;
		switch (action) {
			case NoAct:
				break;
			case Exit0:
				exit(0);
				break;
			case Exit1:
				exit(1);
				break;
			case Abort:
				abort();
				break;
		}

		return;
	}

	void addPrefix(string prefix)
	{
		MSG = prefix + MSG;
		return;
	}
};

class ErrorMessage {
	queue<ErrorInfo *> Buffer;

public:
	ErrorMessage() { }

	void
	newMessage(ErrorInfo *info)
	{
		Buffer.push(info);
		return;
	}

	void
	popAll(ostream& strm)
	{
		while (Buffer.size() > 0)
		{
			Buffer.front()->doPrint(strm, ErrorInfo::NoAct);
			delete Buffer.front();
			Buffer.pop();
		}

		return;
	}

	void
	popInDefaultAction(ostream& strm)
	{
		while (Buffer.size() > 0)
		{
			Buffer.front()->doPrint(strm);
			delete Buffer.front();
			Buffer.pop();
		}

		return;
	}

	void
	popAllAndExit1(ostream& strm)
	{
		while (Buffer.size() > 0)
		{
			Buffer.front()->doPrint(strm);
			delete Buffer.front();
			Buffer.pop();
		}

		exit(1);
		return;
	}

	void
	setTopLineNumber(int line_number)
	{
		stringstream strm;
		strm << line_number << ": ";

		Buffer.front()->addPrefix(strm.str());
		Buffer.front()->addPrefix("line ");
		return;
	}
};

#endif
