#include <v8.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#define DECLARE_FN(name)							\
	v8::Handle<v8::Value> name(const v8::Arguments& args)

#define SET_METHOD(obj, name, callback)						\
	obj->Set(	v8::String::NewSymbol(name),				\
			v8::FunctionTemplate::New(callback)->GetFunction())

#define SET_VALUE(obj, name, value, type)					\
	obj->Set(	v8::String::NewSymbol(name)				\
			v8::type::New(value))

#define DIE(message)								\
	return v8::ThrowException(v8::String::New(message))


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

int RunMain(int argc, char* argv[]){
	v8::HandleScope handle_scope;
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
	SET_METHOD(global, "echo",	Print	);
	SET_METHOD(global, "open",	Read	);
	SET_METHOD(global, "save",	Save	);
	SET_METHOD(global, "import",	Load	);
	SET_METHOD(global, "exit",	Quit	);
	SET_METHOD(global, "shell",	ShellEx	);
	v8::Handle<v8::Context> context = v8::Context::New(NULL, global);
	v8::Context::Scope context_scope(context);
	if (!importFile("makefile.js", "init")){
		printf("makejs: *** Couldn't execute makefile.js. Stop.\n");
		return 1;
	}
	if (argc < 2){ // TODO: This doesn't need to be run in javascript, really.
		ExecuteChar("if (all & all.constructor === Function) all; else echo('makejs: *** function all not found in makefile.js!\\\n')", "init");
	}
	for (int i=1; i<argc; i++){ // TODO: Assign flags to javascript.
		const char* str = argv[i];
		v8::HandleScope handle_scope;
		char *argu;
		argu = new char[32];
		sprintf(argu, "%s();", str);
		ExecuteChar(argu, "unnamed");
	}
	ExecuteChar("if (typeof onfinish === 'function') onfinish();", "finish");
	return 0;*/
}

int main(int argc, char* argv[]){
	RunMain(argc, argv);
	v8::V8::Dispose();
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

v8::Handle<v8::Value> ShellEx(const v8::Arguments& args){
	if (args.Length() < 1){
		DIE("Bad parameters");
	}
	v8::String::Utf8Value comm(args[0]);
	if (*comm == NULL){
		DIE("Error in shell command");
	}
	int res = system(*comm);
	return v8::Number::New(res);
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
			sprintf(libfile, "usr/share/makejs/%s", *file);
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
		printf("%s\n", exception_string);
	} else {
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		printf("%s:%i: %s\n", filename_string, linenum, exception_string);
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char* sourceline_string = ToCString(sourceline);
		printf("%s\n", sourceline_string);
		int start = message->GetStartColumn();
		for (int i=0; i<start; i++){
			printf(" ");
		}
		int end = message->GetEndColumn();
		for (int i=start; i<end; i++){
			printf("^");
		}
		printf("\n");
		v8::String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0){
			const char* stack_trace_string = ToCString(stack_trace);
			printf("%s\n", stack_trace_string);
		}
	}
}
