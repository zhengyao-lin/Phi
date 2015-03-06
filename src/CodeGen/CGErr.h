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
CGERR_setLineNum(CodeGenContext& context, int line_number)
{
	context.messages.setTopLineNumber(line_number);
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
