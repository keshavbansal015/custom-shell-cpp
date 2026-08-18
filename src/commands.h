#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <vector>

using namespace std;
inline const vector<string> BUILTINS = {"echo", "type", "pwd", "cd", "exit"};
string findInPATH(const string &command);
string typeCommand(const string &command);
string execute(string &command);
string pwd();
string cd(string directory);
string echo(string message);
string evaluateCommand(string command);
void printOutput(string output);

#endif

