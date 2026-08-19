#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
inline const vector<string> BUILTINS = {"echo",     "type", "pwd", "cd",
                                        "complete", "jobs", "exit"};

extern unordered_map<string, string> completion_specs;
string findInPATH(const string &command);
string typeCommand(const string &command);
string execute(const string &command);
string pwd();
string cd(const string &directory);
string echo(const string &message);
string evaluateCommand(const string &command);
string completeCommand(const string &command);
void printOutput(const string &output);

#endif
