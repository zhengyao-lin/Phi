#ifndef _CGERR_H_
#define _CGERR_H_

#include "ErrorMsg/EMCore.h"
#include "CGAST.h"

using namespace std;

inline void
CGERR_showAllAndExit1(CodeGenContext& context)
{
	context.messages.popAllAndExit1(cerr);
	return;
}

inline void
CGERR_showAllMsg(CodeGenContext& context)
{
	context.messages.popInDefaultAction(cerr);
	return;
}

inline void
CGERR_setLineNum(CodeGenContext& context, int lineno)
{
	context.messages.setTopLineNumber(lineno);
	return;
}

inline void
CGERR_setLineNum(CodeGenContext& context, int lineno, char *file_name)
{
	context.messages.setTopLineNumber(lineno);
	context.messages.setTopFileName(file_name);
	return;
}

inline void
CGERR_Unknown_Type_Name(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unknown type name \"$(name)\"", name));
	return;
}

inline void
CGERR_Undeclared_Identifier(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Undeclared identifier \"$(name)\"", name));
	return;
}

inline void
CGERR_Calling_Non_Function_Value(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Calling non-function value"));
	return;
}

inline void
CGERR_Get_Non_Structure_Type_Field(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Get non-structure type field"));
	return;
}

inline void
CGERR_Failed_To_Find_Field_Name(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Failed to find field name \"$(name)\"", name));
	return;
}

#define BUFFER_SIZE (1000)
inline void
CGERR_Too_Huge_Integer(CodeGenContext& context, int size)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d", size);

	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Too huge integer (with size of $(size))", buffer));
	return;
}

inline void
CGERR_Integer_Type_With_Size_Of_Zero(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Integer type with size of zero"));
	return;
}

inline void
CGERR_Unknown_Binary_Operation(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unknown binary operation"));
	return;
}

inline void
CGERR_FP_Value_With_Shift_Operation(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "FP value with shift operation (cast to int first?)"));
	return;
}

inline void
CGERR_Get_Non_Array_Element(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Get non-array value's element"));
	return;
}

inline void
CGERR_Get_Non_Resident_Value_Address(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Get non-resident value's address"));
	return;
}

inline void
CGERR_Function_Call_As_LValue(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Function call cannot be a lvalue"));
	return;
}

inline void
CGERR_Unknown_Unary_Operation(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unknown unary operation"));
	return;
}

inline void
CGERR_Non_Constant_Array_Size(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Non-constant array size"));
	return;
}

inline void
CGERR_Get_Sizeof_Void(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Cannot get size of void"));
	return;
}

inline void
CGERR_Get_Alignof_Void(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Cannot get align of void"));
	return;
}

inline void
CGERR_Unknown_Struct_Type(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unknown structure type name \"$(name)\"", name));
	return;
}

inline void
CGERR_Unknown_Union_Type(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unknown union type name \"$(name)\"", name));
	return;
}

inline void
CGERR_Suppose_To_Be_Struct_Type(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Type \"$(name)\" is suppose to be structure type (add struct keyword)", name));
	return;
}

inline void
CGERR_Suppose_To_Be_Union_Type(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Type \"$(name)\" is suppose to be union type (add union keyword)", name));
	return;
}

inline void
CGERR_Initializer_Cannot_Be_In_Struct(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Field in structure cannot have initializer (ignore)"));
	return;
}

inline void
CGERR_Initializer_Cannot_Be_In_Union(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Field in union cannot have initializer (ignore)"));
	return;
}

inline void
CGERR_Nesting_Function(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Nesting function is not allowed"));
	return;
}

inline void
CGERR_External_Variable_Is_Not_Constant(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Initializer of external variable is not a compiler-time constant"));
	return;
}

inline void
CGERR_Unsupport_Integer_Bitwidth_For_Data_Array(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unsupport integer bit width for data array"));
	return;
}

inline void
CGERR_Duplicate_Struct_Type_Name(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Duplicate structure type name"));
	return;
}

inline void
CGERR_Duplicate_Union_Type_Name(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Duplicate union type name"));
	return;
}

inline void
CGERR_Missing_Return_Statement(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Missing return statement"));
	return;
}

inline void
CGERR_Redefinition_Of_Struct(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Redefinition of struct \"$(name)\"", name));
	return;
}

inline void
CGERR_Redefinition_Of_Union(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Redefinition of union \"$(name)\"", name));
	return;
}

inline void
CGERR_Unassignable_LValue(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Unassignable left value"));
	return;
}

inline void
CGERR_Useless_Param(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Useless parameter without a name"));
	return;
}

inline void
CGERR_Conflicting_Type(CodeGenContext& context, const char *name, int arg_no)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d%s", arg_no, (arg_no % 10 == 1 ? "st" : (arg_no % 10 == 2 ? "nd" : (arg_no % 10 == 3 ? "rd" : "th"))));

	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Conflicting type for \"$(name)\" (the $(num) argument)", name, buffer));
	return;
}

inline void
CGERR_Void_Type_Param(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Parameter with void type"));
	return;
}

inline void
CGERR_Void_Should_Be_The_Only_Param(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "void must be the first and only parameter if specified"));
	return;
}

inline void
CGERR_Redefinition_Of_Function(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Redefinition of function \"$(name)\"", name));
	return;
}

inline void
CGERR_Inc_Dec_Unassignable_Value(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Use inc/dec operator on unassignable value"));
	return;
}

inline void
CGERR_Invalid_Main_Function_Return_Type(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Note, true, ErrorInfo::NoAct,
											  "main function return type is supposed to be int"));
	return;
}

inline void
CGERR_Invalid_Use_Of_Void(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Invalid use of incompelete type void"));
	return;
}

inline void
CGERR_Invalid_Use_Of_Incompelete_Type(CodeGenContext& context, const char *name)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Invalid use of incompelete type \"$(name)\"", name));
	return;
}

inline void
CGERR_Missing_Main_Function(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "The function named \"main\" is missing, exiting..."));
	return;
}

inline void
CGERR_Non_Constant_Initializer(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Warning, true, ErrorInfo::NoAct,
											  "Non-constant initializer will be initialized in constructor which can't work in JIT mode"));
	return;
}

inline void
CGERR_Continue_Without_Iteration(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Used continue statement without iteration statement"));
	return;
}

inline void
CGERR_Break_Without_Iteration(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Used break statement without iteration statement or switch statement"));
	return;
}

inline void
CGERR_Unable_Match_Arguments(CodeGenContext& context)
{
	context.messages.newMessage(new ErrorInfo(ErrorInfo::Error, true, ErrorInfo::Exit1,
											  "Cannot match any function for function call"));
	return;
}

#endif
