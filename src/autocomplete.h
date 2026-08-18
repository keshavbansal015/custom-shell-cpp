#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H
#include <string>
#include <vector>
using namespace std;

vector<string> getMatchingCommands(const string &prefix);
string getLongestCommonPrefix(const vector<string> &matches);
vector<string> getMatchingPaths(const string &active_token);
#endif