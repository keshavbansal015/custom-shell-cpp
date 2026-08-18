#include "commands.h"
#include "parser.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

std::string pwd() {
  filesystem::path cwd = std::filesystem::current_path();
  return cwd.string();
}

std::string cd(std::string directory) {
  if (directory == "") {
    return "";
  }

  string targetDir = directory;
  if (directory == "~") {
    char *home = getenv("HOME");
    targetDir = home ? home : "";
  } else if (directory == "-") {
    char *oldpwd = getenv("OLDPWD");
    if (!oldpwd) {
      return "cd: OLDPWD not set";
    }
    targetDir = oldpwd;
  }

  // Save old path to OLDPWD if chdir succeeds
  error_code ec;
  string oldPath = filesystem::current_path(ec).string();

  if (chdir(targetDir.c_str()) == 0) {
    setenv("OLDPWD", oldPath.c_str(), 1);
    return "";
  }

  return "cd: " + directory + ": No such file or directory";
}

std::string findInPATH(const std::string &command) {
  char *path = getenv("PATH");
  string pathStr = path;

  size_t prevPos = 0;
  size_t currPos = pathStr.find(":");

  // Traverse through all the directories in the PATH
  while (currPos != string::npos) {
    string currDir = pathStr.substr(prevPos, currPos - prevPos);
    string fullPath = currDir + "/" + command;

    if (access(fullPath.c_str(), F_OK) == 0 &&
        access(fullPath.c_str(), X_OK) == 0) {
      return fullPath;
    }

    prevPos = currPos + 1;
    currPos = pathStr.find(":", prevPos);
  }
  return "";
}

std::string typeCommand(const std::string &command) {
  // check if the command is a shell builtin
  for (string shellCommand : BUILTINS) {
    if (command.substr(0, shellCommand.length()) == shellCommand) {
      return shellCommand + " is a shell builtin";
    }
  }

  string output = findInPATH(command);
  if (output == "") {
    return command + ": not found";
  }
  return command + " is " + output;
}

std::string execute(std::string &command) {
  vector<string> tokens = parseArguments(command);
  if (tokens.empty()) {
    return "";
  }

  // converting vector of string to array of char*
  std::vector<char*> args;
  // char **args = new char *[tokens.size() + 1];
  for (size_t i = 0; i < tokens.size(); i++) {
    args.push_back(const_cast<char *>(tokens[i].c_str()));
  }
  args.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args.data());
    // If execvp returns, it failed
    cerr << args[0] << ": command not found" << endl;
    exit(127);
  } else if (pid < 0) {
    return "Error in forking";
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
  return "";
}

std::string echo(std::string message) { return cleanInput(message); }

std::string evaluateCommand(std::string command) {
  if (command == "exit") {
    exit(0);
  } else if (command.substr(0, 4) == "echo" && command[4] == ' ') {
    return echo(command.substr(5));
  } else if (command.substr(0, 4) == "type" && command[4] == ' ') {
    return typeCommand(command.substr(5));
  } else if (command == "pwd") {
    return pwd();
  } else if (command.substr(0, 2) == "cd" && command[2] == ' ') {
    return cd(command.substr(3));
  } else {
    return execute(command);
  }
}

void printOutput(std::string output) {
  if (!output.empty()) {
    cout << output << endl;
  }
}