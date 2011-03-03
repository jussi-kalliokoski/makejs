#include <v8.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#define DECLARE_FN(name)							\
	v8::Handle<v8::Value> name(const v8::Arguments& args)

#define SET_METHOD(obj, name, callback)						\
	obj->Set(	v8::String::NewSymbol(name),				\
			v8::FunctionTemplate::New(callback)->GetFunction())

#define SET_FUNCTION(obj, name, callback)					\
	obj->Set(	v8::String::NewSymbol(name),				\
			v8::FunctionTemplate::New(callback))

#define SET_VALUE(obj, name, value, type)					\
	obj->Set(	v8::String::NewSymbol(name),				\
			v8::type::New(value))

#define SET_OBJ(obj, name, value)						\
	obj->Set(	v8::String::NewSymbol(name),				\
			value)

#define CALL_METHOD(obj, name, owner, argc, argv)				\
	v8::Function::Cast(							\
		*obj->Get(							\
			v8::String::NewSymbol(name)))->Call(			\
				owner, argc, argv				\
	)								

#define DIE(message)								\
	return v8::ThrowException(v8::String::New(message))


int RunMain(int argc, char* argv[]);
bool ExecuteChar(const char *code, const char *name);
bool importFile(const char *file, const char *name);
void ReportException(v8::TryCatch* handler);
bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions);
v8::Handle<v8::String> ReadFile(const char* name);
v8::Handle<v8::String> SaveFile(const char* name, const char* content);

DECLARE_FN(Print);
DECLARE_FN(Read);
DECLARE_FN(Save);
DECLARE_FN(Load);
DECLARE_FN(Quit);
DECLARE_FN(ShellEx);
DECLARE_FN(FileProperties);

int main(int argc, char* argv[]){
	int escCode = RunMain(argc, argv);
	v8::V8::Dispose();
	return escCode;
}

int RunMain(int argc, char* argv[]){
	timeval startTime, endTime;
	int timeSuccess = gettimeofday(&startTime, NULL);

	v8::HandleScope handle_scope;
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

	SET_FUNCTION(global, "open",	Read	);
	SET_FUNCTION(global, "save",	Save	);
	SET_FUNCTION(global, "import",	Load	);
	SET_FUNCTION(global, "exit",	Quit	);
	SET_FUNCTION(global, "shell",	ShellEx	);

	v8::Handle<v8::Context> context = v8::Context::New(NULL, global);
	v8::Context::Scope context_scope(context);
	v8::Handle<v8::Object> globalObj = context->Global();

	v8::Handle<v8::Function> logFunction = v8::FunctionTemplate::New(Print)->GetFunction();
	SET_OBJ(globalObj, "echo", logFunction);
	v8::Handle<v8::Object> consoleObj = v8::Object::New();
	v8::Handle<v8::Object> makejsObj = v8::Object::New();
	SET_OBJ(consoleObj, "log", logFunction);
	SET_OBJ(globalObj, "console", consoleObj);
	SET_OBJ(globalObj, "makejs", makejsObj);
	SET_METHOD(globalObj, "fileProperties", FileProperties);

	v8::Handle<v8::Value>* flagsArguments;

	v8::Handle<v8::Array> rules = v8::Array::New();
	SET_OBJ(makejsObj, "rules", rules);
	v8::Handle<v8::Array> flags = v8::Array::New();
	SET_OBJ(makejsObj, "flags", flags);
	v8::Handle<v8::Array> rawFlags = v8::Array::New();
	SET_OBJ(makejsObj, "rawFlags", rawFlags);
	v8::Handle<v8::Object> argumentedFlags = v8::Object::New();
	SET_OBJ(makejsObj, "argumentedFlags", argumentedFlags);
	SET_VALUE(makejsObj, "started", (startTime.tv_sec * 1000000.0) + startTime.tv_usec, Number);

	bool specifyingRules = true;
	int ruleCount = 0;
	int flagCount = 0;

	for (int i=1; i<argc; i++){
		const char* str = argv[i];
		v8::HandleScope handle_scope;
		rawFlags->Set(v8::Number::New(i-1), v8::String::New(str));
		if (strncmp(str, "-", 1) == 0){
			specifyingRules = false;
			flags->Set(v8::Number::New(flagCount++), v8::String::New(str));
			v8::Handle<v8::Array> argumentedFlag = v8::Array::New();
			SET_OBJ(argumentedFlags, str, argumentedFlag);
			int argCount = 0;
			while (i+argCount<argc-1 && strncmp(argv[i+argCount+1], "-", 1) != 0){
				argumentedFlag->Set(v8::Number::New(argCount++), v8::String::New(argv[i+argCount+1]));
			}
			i += argCount;
		} else if (specifyingRules){
			rules->Set(v8::Number::New(ruleCount++), v8::String::New(str));
		}
	}
	if (ruleCount == 0){
		rules->Set(v8::Number::New(ruleCount++), v8::String::New("all"));
	}
	

	if (!importFile("makefile.js", "init")){
		printf("makejs: *** Couldn't execute makefile.js. Stop.\n");
		return 1;
	}

	v8::TryCatch try_catch;

	for (int i=0; i<ruleCount; i++){
		v8::String::Utf8Value rule(rules->Get(v8::Number::New(i)));
		if (globalObj->Get(v8::String::NewSymbol(*rule))->IsFunction()){
			CALL_METHOD(globalObj, *rule, globalObj, 0, flagsArguments);
			if (try_catch.HasCaught()){
				ReportException(&try_catch);
				return 1;
			}
		} else {
			printf("makejs: *** Rule %s not specified in the makefile. Stop.\n", *rule);
			return 1;
		}
	}

	if (globalObj->Get(v8::String::NewSymbol("onfinish"))->IsFunction()){
		CALL_METHOD(globalObj, "onfinish", globalObj, 0, flagsArguments);
		if (try_catch.HasCaught()){
			ReportException(&try_catch);
			return 1;
		}
	}

	gettimeofday(&endTime, NULL);
	double stTime = startTime.tv_sec + (startTime.tv_usec / 1000000.0);
	double enTime = endTime.tv_sec + (endTime.tv_usec / 1000000.0);

	printf("makejs: finished succesfully in %.21f seconds\n", enTime - stTime);

	return 0;
}

bool ExecuteChar(const char *code, const char *name){
	v8::HandleScope handle_scope;
	v8::Handle<v8::String> vcode = v8::String::New(code);
	v8::Handle<v8::String> vname = v8::String::New(name);
	ExecuteString(vcode, vname, false, true);
}

const char* ToCString(const v8::String::Utf8Value& value){
	return *value ? *value : "<cannot convert to primitive value>";
}

v8::Handle<v8::Value> Print(const v8::Arguments& args){
	bool first = true;
	for (int i=0; i<args.Length(); i++){
		if (first){
			first = false;
		} else {
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);
	}
	printf("\n");
	fflush(stdout);
	return v8::Undefined();
}

v8::Handle<v8::Value> Read(const v8::Arguments& args){
	if (args.Length() != 1){
		DIE("Bad parameters");
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL){
		DIE("Error loading file");
	}
	v8::Handle<v8::String> source = ReadFile(*file);
	if (source.IsEmpty()){
		DIE("Error loading file");
	}
	return source;
}

v8::Handle<v8::Value> Save(const v8::Arguments& args){
	if (args.Length() != 2){
		DIE("Bad parameters");
	}
	v8::String::Utf8Value file(args[0]);
	v8::String::Utf8Value content(args[1]);
	if (*file == NULL || *content == NULL){
		DIE("Error loading file");
	}
	return SaveFile(*file, *content);
}

v8::Handle<v8::Value> FileProperties(const v8::Arguments& args){
	if (args.Length() < 1){
		DIE("Bad parameters");
	}
	struct stat fileInfo;
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL){
		DIE("Error loading file");
	}
	if (stat(*file, &fileInfo) != 0){
		DIE("Error in \"stat\"");
	}
	v8::Handle<v8::Object> fileData = v8::Object::New();
	SET_VALUE(fileData, "mode", fileInfo.st_mode, Number);
	SET_VALUE(fileData, "lastAccess", fileInfo.st_atime, Number);
	SET_VALUE(fileData, "lastModified", fileInfo.st_mtime, Number);
	SET_VALUE(fileData, "lastChanged", fileInfo.st_ctime, Number);
	SET_VALUE(fileData, "size", fileInfo.st_size, Number);
	return fileData;
}

v8::Handle<v8::Value> ShellEx(const v8::Arguments& args){
	if (args.Length() < 1){
		DIE("Bad parameters");
	}
	bool silent = false;
	if (args.Length() == 2){
		silent = args[1]->IsTrue();
	}
	v8::String::Utf8Value comm(args[0]);
	if (*comm == NULL){
		DIE("Error in shell command");
	}
	if (!silent){
		printf("$ %s\n", *comm);
	}

	std::string data;
	FILE *stream;
	int MAX_BUFFER = 256;
	char buffer[MAX_BUFFER];

	stream = popen(*comm, "r");
	if (!stream){
		DIE("Failed to open stream");
	}
	while(!feof(stream)){
		if ( fgets(buffer, MAX_BUFFER, stream) != NULL ){
			data.append(buffer);
		}
	}
	int res = pclose(stream);

	const char *datastr = data.c_str();

	if (!silent){
		printf("%s", datastr);
	}
	
	if (res != 0){
		char *str = new char[32];
		sprintf(str, "Shell exited with error code %i", res);
		DIE(str);
	}
	return v8::String::New(datastr);
}

v8::Handle<v8::Value> Load(const v8::Arguments& args){
	for (int i=0; i<args.Length(); i++){
		v8::HandleScope handle_scope;
		v8::String::Utf8Value file(args[i]);
		if (*file == NULL){
			DIE("Error loading file");
		}
		v8::Handle<v8::String> source = ReadFile(*file);
		if (source.IsEmpty()){
			char *libfile = new char[32];
			sprintf(libfile, "/usr/share/makejs/%s", *file);
			source = ReadFile(libfile);
		}
		if (source.IsEmpty()){
			DIE("Error loading file");
		}
		if (!ExecuteString(source, v8::String::New(*file), false, false)){
			DIE("Error executing file");
		}
	}
	return v8::Undefined();
}

bool importFile(const char *file, const char *name){
	v8::HandleScope handle_scope;
	v8::Handle<v8::String> source = ReadFile(file);
	if (source.IsEmpty()){
		return false;
	}
	if (!ExecuteString(source, v8::String::New(name), true, false)){
		return false;
	}
	return true;
}

v8::Handle<v8::Value> Quit(const v8::Arguments& args){
	int exit_code = args[0]->Int32Value();
	exit(exit_code);
	return v8::Undefined();
}

v8::Handle<v8::String> SaveFile(const char* name, const char* content){
	std::ofstream fileOut;
	fileOut.open(name);
	fileOut << content;
	fileOut.close();
	return v8::String::New("0");
}

v8::Handle<v8::String> ReadFile(const char* name){
	FILE* file = fopen(name, "rb");
	if (file == NULL){
		return v8::Handle<v8::String>();
	}
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);
	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i=0; i<size;){
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);
	v8::Handle<v8::String> result = v8::String::New(chars, size);
	delete[] chars;
	return result;
}

bool ExecuteString(v8::Handle<v8::String> source,
			v8::Handle<v8::Value> name,
			bool print_result,
			bool report_exceptions){
	v8::HandleScope handle_scope;
	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	if (script.IsEmpty()){
		if (report_exceptions){
			ReportException(&try_catch);
		}
		return false;
	} else {
		v8::Handle<v8::Value> result = script->Run();
		if (result.IsEmpty()){
			if (report_exceptions){
				ReportException(&try_catch);
			}
			return false;
		} else {
			if (print_result && !result->IsUndefined()){
				v8::String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}
}

void ReportException(v8::TryCatch* try_catch){
	v8::HandleScope handle_scope;
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Handle<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()){
		printf("makejs: *** %s. Stop.\n", exception_string);
	} else {
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		printf("makejs: *** %s on line %i: %s. Stop\n", filename_string, linenum, exception_string);
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		v8::String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0){
			const char* stack_trace_string = ToCString(stack_trace);
			printf("%s\n", stack_trace_string);
		}
	}
}
