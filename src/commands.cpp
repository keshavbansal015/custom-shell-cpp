#include "commands.h"
#include "parser.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

string pwd() {
  filesystem::path cwd = filesystem::current_path();
  return cwd.string();
}

string cd(const string &directory) {
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

string findInPATH(const string &command) {
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

string typeCommand(const string &command) {
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

string execute(const string &command) {
  string cmd = command;
  vector<string> tokens = parseArguments(cmd);
  if (tokens.empty()) {
    return "";
  }

  // converting vector of string to array of char*
  vector<char *> args;
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
string echo(const string &message) { return cleanInput(message); }

// Map to store command completion specifications
unordered_map<string, string> completion_specs;

string completeCommand(const string &command) {
  vector<string> tokens = parseArguments(command);
  if (tokens.empty())
    return "";

  if (tokens[0] == "-p") {
    if (tokens.size() < 2) {
      return "complete: usage: complete -p command";
    }
    string cmd_name = tokens[1];
    if (completion_specs.count(cmd_name)) {
      return "complete -C '" + completion_specs[cmd_name] + "' " + cmd_name;
    } else {
      return "complete: " + cmd_name + ": no completion specification";
    }
  } else if (tokens[0] == "-C") {
    if (tokens.size() < 3) {
      return "complete: usage: complete -C completer command";
    }
    string completer = tokens[1];
    string cmd_name = tokens[2];
    completion_specs[cmd_name] = completer;
    return "";
  }

  return "complete: unsupported option";
}

string evaluateCommand(const string &command) {
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
  } else if (command.substr(0, 8) == "complete" && command[8] == ' ') {
    return completeCommand(command.substr(9));
  } else {
    return execute(command);
  }
}

void printOutput(const string &output) {
  if (!output.empty()) {
    cout << output << endl;
  }
}