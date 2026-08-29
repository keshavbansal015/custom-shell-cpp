#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>

using namespace std;

// TokenInfo example: 
//  Command: echo "Hello" -> base_prefix = "echo ", active_token = "Hello", is_command = true
//  Command: echo "Hello " -> base_prefix = "echo ", active_token = "Hello ", is_command = false, so path completion
//  Command: cd /usr/b -> base_prefix = "cd ", active_token = "/usr/b", is_command = false, so path completion
//  Command: cd /usr/bin/ -> base_prefix = "cd /usr/bin/", active_token = "", is_command = false, so path completion
struct TokenInfo {
  string base_prefix;
  string active_token;
  bool is_command;
};


struct CommandRedirection {
  string clean_command;
  string stdout_file;
  string stderr_file;
  bool redirect_stdout;
  bool redirect_stderr;
  bool append_stdout;
  bool append_stderr;
};

vector<string> stringSplit(const string &str, char delimiter);
TokenInfo parseCompletionTarget(const string &command);
string cleanPathToken(const string &token);
void splitPath(const string &path, string &dir, string &prefix);
string getVariableValue(const string &var_name);
vector<string> parseArguments(const string &message);
string cleanInput(const string &message);
CommandRedirection parseRedirection(const string &command);
vector<string> splitPipeline(const string &command);

#endif
