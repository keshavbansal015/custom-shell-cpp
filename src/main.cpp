#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>

#define SHELL_COMMANDS {"echo", "type", "pwd", "cd", "exit"}
#define SHELL_PROMPT "$ "

std::string readInput() {
  using namespace std;
  cout << SHELL_PROMPT;
  string command;
  getline(cin, command);
  return command;
}

std::string findInPATH(std::string command) {
  using namespace std;
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

std::string typeCommand(std::string command) {
  using namespace std;

  // check if the command is a shell builtin
  for (string shellCommand : SHELL_COMMANDS) {
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
std::string cleanInput(std::string message) {
  using namespace std;
  vector<string> args;
  string current_arg = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  bool has_arg = false;

  for (size_t i = 0; i < message.length(); ++i) {
    char c = message[i];
    if (in_single_quotes) {
      if (c == '\'') {
        in_single_quotes = false;
      } else {
        current_arg += c;
      }
    } else if (in_double_quotes) {
      if (c == '"') {
        in_double_quotes = false;
      } else if (c == '\\' && i + 1 < message.length() &&
                 (message[i + 1] == '"' || message[i + 1] == '\\' ||
                  message[i + 1] == '$' || message[i + 1] == '\n')) {
        current_arg += message[i + 1];
        i++;
      } else {
        current_arg += c;
      }
    } else {
      if (c == '\'') {
        in_single_quotes = true;
        has_arg = true;
      } else if (c == '"') {
        in_double_quotes = true;
        has_arg = true;
      } else if (c == ' ') {
        if (has_arg) {
          args.push_back(current_arg);
          current_arg = "";
          has_arg = false;
        }
      } else if (c == '\\' && i + 1 < message.length()) {
        current_arg += message[i + 1];
        i++;
        has_arg = true;
      } else {
        current_arg += c;
        has_arg = true;
      }
    }
  }
  if (has_arg) {
    args.push_back(current_arg);
  }

  string result = "";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) {
      result += " ";
    }
    result += args[i];
  }
  return result;
}

std::string execute(std::string command) {
  using namespace std;

  // using execvp
  command = cleanInput(command);

  if (command == "") {
    return "";
  }

  vector<string> tokens =
      command | views::split(' ') | ranges::to<vector<string>>();

  // converting vector of string to array of char*
  char **args = new char *[tokens.size() + 1];
  for (size_t i = 0; i < tokens.size(); i++) {
    args[i] = (char *)tokens[i].c_str();
  }
  args[tokens.size()] = nullptr;

  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args);
    // If execvp returns, it failed
    cerr << args[0] << ": command not found" << endl;
    exit(127);
  } else if (pid < 0) {
    delete[] args;
    return "Error in forking";
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
  delete[] args;
  return "";
}

std::string pwd() {
  using namespace std;
  filesystem::path cwd = std::filesystem::current_path();
  return cwd.string();
}

std::string cd(std::string directory) {
  using namespace std;
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

std::string echo(std::string message) { return cleanInput(message); }

std::string evaluateCommand(std::string command) {
  using namespace std;
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
  using namespace std;
  if (!output.empty()) {
    cout << output << endl;
  }
}

void shellLoop() {
  using namespace std;
  while (true) {
    string command = readInput();
    string output = evaluateCommand(command);
    printOutput(output);
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  shellLoop();
}
