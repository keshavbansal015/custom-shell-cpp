#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H
#include <string>
#include <vector>
using namespace std;

vector<string> getMatchingCommands(const string &prefix);
string getLongestCommonPrefix(const vector<string> &matches);
vector<string> getMatchingPaths(const string &active_token);
vector<string> runCompleter(const string &completer_path, const string &cmd_name, const string &current_word, const string &previous_word, const string &full_line);
#endif