// Nicely ripped off from an example, make original source code later...

#include <v8.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>


void RunShell(v8::Handle<v8::Context> context);
bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions);
bool ExecuteChar(const char *code, const char *name);
v8::Handle<v8::Value> Print(const v8::Arguments& args);
v8::Handle<v8::Value> Read(const v8::Arguments& args);
v8::Handle<v8::Value> Save(const v8::Arguments& args);
v8::Handle<v8::Value> Load(const v8::Arguments& args);
bool importFile(const char *file, const char *name);
v8::Handle<v8::Value> Quit(const v8::Arguments& args);
v8::Handle<v8::Value> Version(const v8::Arguments& args);
v8::Handle<v8::Value> ShellEx(const v8::Arguments& args);
v8::Handle<v8::String> ReadFile(const char* name);
v8::Handle<v8::String> SaveFile(const char* name, const char* content);
void ReportException(v8::TryCatch* handler);


int RunMain(int argc, char* argv[]) {
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
  v8::HandleScope handle_scope;
  v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  global->Set(v8::String::New("echo"), v8::FunctionTemplate::New(Print));
  global->Set(v8::String::New("open"), v8::FunctionTemplate::New(Read));
  global->Set(v8::String::New("save"), v8::FunctionTemplate::New(Save));
  global->Set(v8::String::New("import"), v8::FunctionTemplate::New(Load));
  global->Set(v8::String::New("exit"), v8::FunctionTemplate::New(Quit));
  global->Set(v8::String::New("version"), v8::FunctionTemplate::New(Version));
  global->Set(v8::String::New("shell"), v8::FunctionTemplate::New(ShellEx));
  v8::Handle<v8::Context> context = v8::Context::New(NULL, global);
  v8::Context::Scope context_scope(context);
  ExecuteChar("var all, onfinish;", "init");
  if(!importFile("makefile.js", "init"))
  {
    printf("makejs: *** Couldn't execute makefile.js. Stop.\n");
    return 1;
  }
  if (argc < 2)
    ExecuteChar("if (all && all.constructor === Function) all(); else echo('makejs: *** function all not found in makefile.js!\\\n')", "init");
  for (int i = 1; i < argc; i++) {
    const char* str = argv[i];
    /*if (strcmp(str, "-") == 1) {
      printf("Warning: unknown flag %s.\nTry --help for options\n", str);
    } else if (strncmp(str, "--", 2) == 0) {
      printf("Warning: unknown flag %s.\nTry --help for options\n", str);
    } else*/
    if (0 == 0) {
      v8::HandleScope handle_scope;
      v8::Handle<v8::String> file_name = v8::String::New("unnamed");
      char *argu;
      argu = new char[32];
      sprintf(argu, "%s();", str);
      v8::Handle<v8::String> source = v8::String::New(argu);
      ExecuteString(source, file_name, false, true);
    }
  }
  ExecuteChar("if (onfinish && onfinish.constructor === Function) onfinish();", "finish");
  return 0;
}



int main(int argc, char* argv[]) {
  RunMain(argc, argv);
  v8::V8::Dispose();
  return 0;
}

bool ExecuteChar(const char *code, const char *name)
{
  v8::HandleScope handle_scope;
  v8::Handle<v8::String> vcode = v8::String::New(code);
  v8::Handle<v8::String> vname = v8::String::New(name);
  ExecuteString(vcode, vname, false, true);
}


const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

v8::Handle<v8::Value> Print(const v8::Arguments& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope;
    if (first) {
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

v8::Handle<v8::Value> Read(const v8::Arguments& args) {
  if (args.Length() != 1) {
    return v8::ThrowException(v8::String::New("Bad parameters"));
  }
  v8::String::Utf8Value file(args[0]);
  if (*file == NULL) {
    return v8::ThrowException(v8::String::New("Error loading file"));
  }
  v8::Handle<v8::String> source = ReadFile(*file);
  if (source.IsEmpty()) {
    return v8::ThrowException(v8::String::New("Error loading file"));
  }
  return source;
}

v8::Handle<v8::Value> Save(const v8::Arguments& args) {
  if (args.Length() != 2) {
    return v8::ThrowException(v8::String::New("Bad parameters"));
  }
  v8::String::Utf8Value file(args[0]);
  v8::String::Utf8Value content(args[1]);
  if (*file == NULL || *content == NULL) {
    return v8::ThrowException(v8::String::New("Error loading file"));
  }
  v8::Handle<v8::String> source = SaveFile(*file, *content);
  return source;
}

v8::Handle<v8::Value> ShellEx(const v8::Arguments& args) {
  if (args.Length() != 1) {
    return v8::ThrowException(v8::String::New("Bad parameters"));
  }
  v8::String::Utf8Value comm(args[0]);
  if (*comm == NULL) {
    return v8::ThrowException(v8::String::New("Error in shell command"));
  }
  int res = system(*comm);
  char *resch = new char[32];
  sprintf(resch, "%i", res);
  v8::Handle<v8::String> result = v8::String::New(resch);
  return result;
}


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
v8::Handle<v8::Value> Load(const v8::Arguments& args) {
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope;
    v8::String::Utf8Value file(args[i]);
    if (*file == NULL) {
      return v8::ThrowException(v8::String::New("Error loading file"));
    }
    v8::Handle<v8::String> source = ReadFile(*file);
    if (source.IsEmpty()) {
      char *libfile;
      libfile = new char[32];
      sprintf(libfile, "/usr/share/makejs/%s", *file);
      source = ReadFile(libfile);
      if (source.IsEmpty()) {
        return v8::ThrowException(v8::String::New("Error loading file"));
      }
    }
    if (!ExecuteString(source, v8::String::New(*file), false, false)) {
      return v8::ThrowException(v8::String::New("Error executing file"));
    }
  }
  return v8::Undefined();
}

bool importFile(const char *file, const char *name)
{
  v8::HandleScope handle_scope;
  v8::Handle<v8::String> source = ReadFile(file);
  if (source.IsEmpty()) {
    return false;
  }
  if (!ExecuteString(source, v8::String::New(name), true, false)){
    return false;
  }
  return true;
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
v8::Handle<v8::Value> Quit(const v8::Arguments& args) {
  // If not arguments are given args[0] will yield undefined which
  // converts to the integer value 0.
  int exit_code = args[0]->Int32Value();
  exit(exit_code);
  return v8::Undefined();
}


v8::Handle<v8::Value> Version(const v8::Arguments& args) {
  return v8::String::New(v8::V8::GetVersion());
}


// Reads a file into a v8 string.
v8::Handle<v8::String> SaveFile(const char* name, const char* content) {
  std::ofstream fileOut;
  fileOut.open(name);
  fileOut << content;
  fileOut.close();
  v8::Handle<v8::String> result = v8::String::New("0");
  return result;
}

v8::Handle<v8::String> ReadFile(const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::Handle<v8::String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  v8::Handle<v8::String> result = v8::String::New(chars, size);
  delete[] chars;
  return result;
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Handle<v8::Context> context) {
  static const int kBufferSize = 256;
  while (true) {
    char buffer[kBufferSize];
    printf("> ");
    char* str = fgets(buffer, kBufferSize, stdin);
    if (str == NULL) break;
    v8::HandleScope handle_scope;
    ExecuteString(v8::String::New(str),
                  v8::String::New("(shell)"),
                  true,
                  true);
  }
  printf("\n");
}


bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope;
  v8::TryCatch try_catch;
  v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
  if (script.IsEmpty()) {
    if (report_exceptions)
      ReportException(&try_catch);
    return false;
  } else {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
      if (report_exceptions)
        ReportException(&try_catch);
      return false;
    } else {
      if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        const char* cstr = ToCString(str);
        printf("%s\n", cstr);
      }
      return true;
    }
  }
}


void ReportException(v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope;
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
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
    for (int i = 0; i < start; i++) {
      printf(" ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      printf("^");
    }
    printf("\n");
    v8::String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      printf("%s\n", stack_trace_string);
    }
  }
}
