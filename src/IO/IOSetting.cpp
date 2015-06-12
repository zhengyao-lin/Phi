#include "IOSetting.h"

void
IOSetting::initMap()
{
	ARG_MAP[ARG_OBJECT] = ObjectFile;
	ARG_MAP[ARG_TARGET_OBJECT] = TargetObj;
	ARG_MAP[ARG_TARGET_ASM] = TargetASM;
	ARG_MAP[ARG_TARGET_IR] = TargetIR;
	ARG_MAP[ARG_TARGET_EXE] = TargetExe;
	return;
}

IOSetting::ArgumentType
IOSetting::getArg(char *arg)
{
	if (ARG_MAP.find(arg) != ARG_MAP.end()) {
		return ARG_MAP[arg];
	}

	return Unknown;
}

string
IOSetting::getRandomString(int length)
{
	int flag, i;
	string ret_str;
	srand(clock());
  
	for (i = 0; i < length - 1; i++)
	{
	    flag = rand() % 3;
	    switch (flag)
	    {
	        case 0:
	            ret_str += 'A' + rand() % 26;
	            break;
	        case 1:
	            ret_str += 'a' + rand() % 26;
	            break;
	        case 2:
	            ret_str += '0' + rand() % 10;
	            break;
	        default:
	            ret_str += 'x';
	            break;
	    }
	}
	return ret_str;
}

string
IOSetting::getTempFilePath()
{
	string file_name = "/tmp/." + getRandomString(16);
	while (isFileExist(file_name + ".tmp")) {
		file_name += ".0";
	}

	return file_name + ".tmp";
}

string
IOSetting::doPreprocess(string file_path)
{
	string tmp_file_path = getTempFilePath();
	string cmd = "gcc -x c " + file_path + " -E >> " + tmp_file_path;

	if (!isFileExist(file_path)) {
		ErrorMessage::tmpError("Cannot find source file: " + file_path);
		delete this;
		exit(0);
	}

	tmp_file_paths->push_back(tmp_file_path);
	int status = system(cmd.c_str());
	if (status) {
		delete this;
		exit(status);
	}

	return tmp_file_path;
}

void
IOSetting::applySetting()
{
	extern char *current_file;
	extern FILE *yyin;
	if (hasInput()) {
		current_file = strdup(input_file.c_str());
		yyin = fopen(doPreprocess(input_file).c_str(), "r");
		if (!yyin) {
			ErrorMessage::tmpError("Cannot find source file: " + input_file);
		}
	} else {
		delete this;
		exit(0);
	}
}

bool
IOSetting::hasInput()
{
	return input_file != "";
}

bool
IOSetting::hasObject()
{
	return object_file != "";
}

string
IOSetting::getObject()
{
	return object_file;
}

bool
IOSetting::targetObj()
{
	return target_object;
}

bool
IOSetting::targetASM()
{
	return target_asm;
}

bool
IOSetting::targetIR()
{
	return target_ir;
}

bool
IOSetting::targetExe()
{
	return target_exe;
}

bool
IOSetting::isIROutput()
{
	return !targetObj() && !targetASM() && !targetExe() && targetIR();
}

string
IOSetting::getFileName(string file)
{
	int i, j;

	for (i = file.length() - 1; i && file[i] != '.'; i--);
	for (j = 0; j < file.length() && file[j] != '/'; j++);
	j = (j == file.length() - 1 ? 0 : j + 1);

	return file.substr(j, i - j);
}

string
IOSetting::getFilePath(string file)
{
	return file.substr(0, file.length() - string(basename(file.c_str())).length());
}

void
IOSetting::doOutput(Module *mod)
{
	TargetMachine::CodeGenFileType output_file_type = TargetMachine::CGFT_Null;
	string tmp_output_name = getObject();

	if (targetObj() || targetExe()) {
		output_file_type = TargetMachine::CGFT_ObjectFile;
	} else if (targetASM()) {
		output_file_type = TargetMachine::CGFT_AssemblyFile;
	}

	if (isIROutput()) {
		if (getObject().empty()) {
			if (input_file.empty()) {
				tmp_output_name = "tmp.ll";
			} else {
				tmp_output_name = getFileName(input_file) + ".ll";
			}
		}

		string error_msg;
		tool_output_file output_file(tmp_output_name.c_str(), error_msg, sys::fs::F_None);
		if (!error_msg.empty()) {
			cerr << error_msg << endl;
			delete this;
			exit(1);
			return;
		}
		output_file.os() << *mod;
		output_file.keep();
	} else if (targetObj() || targetASM() || targetExe()) {
		string error_str;
		const Target *target = TargetRegistry::lookupTarget(
								sys::getDefaultTargetTriple(), error_str);
		if (target == NULL) {
			cout << error_str << endl;
			delete this;
			exit(1);
			return;
		}
		TargetOptions target_options;
		TargetMachine *target_machine = target->createTargetMachine(
									   sys::getDefaultTargetTriple(),
									   sys::getHostCPUName(), "",
									   target_options);

		if (getObject().empty()) {
			if (input_file.empty()) {
				if (targetObj() || targetExe()) {
					tmp_output_name = "tmp.o";
				} else {
					tmp_output_name = "tmp.s";
				}
			} else {
				if (targetObj() || targetExe()) {
					tmp_output_name = getFileName(input_file) + ".o";
				} else {
					tmp_output_name = getFileName(input_file) + ".s";
				}
			}
		} else if (targetExe()) {
			tmp_output_name += ".tmp";
		}

		string error_str2;
		tool_output_file ouput_tool(tmp_output_name.c_str(), error_str2, sys::fs::F_None);
		if (!error_str2.empty()) {
			cout << error_str2 << endl;
			return;
		}
		PassManager pass_m;
		//passManager.add(dataLayout);
		formatted_raw_ostream fos(ouput_tool.os());
		target_machine->addPassesToEmitFile(pass_m, fos, output_file_type);
		pass_m.run(*mod);
		ouput_tool.keep();
	}

	if (targetExe()) {
		cout << getObject() << endl;
		string cmd = "gcc " + tmp_output_name + " -o "
					 + (getObject().empty()
						? "a.out"
						: getObject());
		tmp_file_paths->push_back(tmp_output_name);
		int status = system(cmd.c_str());

		if (isFileExist(tmp_output_name)) {
			remove(tmp_output_name.c_str());
		}

		if (status) {
			delete this;
			exit(status);
		}
	}
}
