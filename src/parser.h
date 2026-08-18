#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>

using namespace std;
struct TokenInfo {
  string base_prefix;
  string active_token;
  bool is_command;
};

vector<string> stringSplit(const string &str, char delimiter);
TokenInfo parseCompletionTarget(const string &command);
string cleanPathToken(const string &token);
void splitPath(const string &path, string &dir, string &prefix);
vector<string> parseArguments(string &message);
string cleanInput(string &message);

#endif
