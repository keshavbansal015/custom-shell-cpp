#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <vector>

inline const std::vector<std::string> BUILTINS = {"echo", "type", "pwd", "cd", "exit"};
std::string findInPATH(const std::string &command);
std::string typeCommand(const std::string &command);
std::string execute(std::string &command);
std::string pwd();
std::string cd(std::string directory);
std::string echo(std::string message);
std::string evaluateCommand(std::string command);
void printOutput(std::string output);

#endif

