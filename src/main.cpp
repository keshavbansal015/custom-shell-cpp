#include <algorithm>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <set>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <cctype>
#include <system_error>

#define BUILTINS {"echo", "type", "pwd", "cd", "exit"}
#define SHELL_PROMPT "$ "

struct termios orig_termios;

// This sets the terminal to its original state
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
// This sets the terminal to raw mode
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ICANON | ECHO); // ICANON -> disable canonical mode, ECHO -> disable echo
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // TCSAFLUSH -> flush the terminal buffer and wait for all output to be written
}

std::vector<std::string> getMatchingCommands(const std::string& prefix) {
  using namespace std;
  set<string> unique_matches;
  vector<string> builtins = BUILTINS;
  
  for (const string& b : builtins) {
    if (b.rfind(prefix, 0) == 0) {
      unique_matches.insert(b);
    }
  }
  
  char *path = getenv("PATH"); // PATH is a colon separated list of directories
  if (path) {
    string pathStr = path; // convert to string
    size_t prevPos = 0;
    size_t currPos = pathStr.find(":");
    while (prevPos < pathStr.length()) {
      string currDir = pathStr.substr(prevPos, currPos - prevPos); // get current directory
      if (!currDir.empty()) {
        try {
          if (filesystem::exists(currDir) && filesystem::is_directory(currDir)) {
            for (const filesystem::directory_entry& entry : filesystem::directory_iterator(currDir)) {
              string filename = entry.path().filename().string();
              if (filename.rfind(prefix, 0) == 0) {
                if (access(entry.path().c_str(), X_OK) == 0) {
                  unique_matches.insert(filename);
                }
              }
            }
          }
        } catch (...) {
          // Ignore directory access errors
        }
      }
      if (currPos == string::npos) {
        break;
      }
      prevPos = currPos + 1;
      currPos = pathStr.find(":", prevPos);
    }
  }
  
  vector<string> matching_commands(unique_matches.begin(), unique_matches.end());
  sort(matching_commands.begin(), matching_commands.end());
  return matching_commands;
}

std::string getLongestCommonPrefix(const std::vector<std::string>& matches) {
  using namespace std;
  if (matches.empty()) return "";
  string prefix = matches[0];
  for (size_t i = 1; i < matches.size(); ++i) {
    while (matches[i].find(prefix) != 0) {
      prefix = prefix.substr(0, prefix.length() - 1);
      if (prefix.empty()) return "";
    }
  }
  return prefix;
}

struct TokenInfo {
  std::string base_prefix;
  std::string active_token;
  bool is_command;
};

TokenInfo parseCompletionTarget(const std::string& command) {
  TokenInfo info;
  info.is_command = true;
  
  if (command.empty()) {
    info.base_prefix = "";
    info.active_token = "";
    return info;
  }

  size_t last_token_start = 0;
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  bool last_was_space = true;
  
  for (size_t i = 0; i < command.length(); ++i) {
    char c = command[i];
    if (in_single_quotes) {
      if (c == '\'') {
        in_single_quotes = false;
      }
    } else if (in_double_quotes) {
      if (c == '"') {
        in_double_quotes = false;
      } else if (c == '\\' && i + 1 < command.length()) {
        i++;
      }
    } else {
      if (c == '\'') {
        in_single_quotes = true;
        if (last_was_space) {
          last_token_start = i;
          last_was_space = false;
        }
      } else if (c == '"') {
        in_double_quotes = true;
        if (last_was_space) {
          last_token_start = i;
          last_was_space = false;
        }
      } else if (c == ' ') {
        last_was_space = true;
      } else if (c == '\\' && i + 1 < command.length()) {
        if (last_was_space) {
          last_token_start = i;
          last_was_space = false;
        }
        i++;
      } else {
        if (last_was_space) {
          last_token_start = i;
          last_was_space = false;
        }
      }
    }
  }

  info.base_prefix = command.substr(0, last_token_start);
  info.active_token = command.substr(last_token_start);
  
  bool has_non_space_before = false;
  for (char c : info.base_prefix) {
    if (c != ' ') {
      has_non_space_before = true;
      break;
    }
  }
  
  if (has_non_space_before || info.active_token.find('/') != std::string::npos) {
    info.is_command = false;
  }
  
  return info;
}

std::string cleanPathToken(const std::string& token) {
  std::string result = "";
  bool in_single = false;
  bool in_double = false;
  for (size_t i = 0; i < token.length(); ++i) {
    char c = token[i];
    if (in_single) {
      if (c == '\'') in_single = false;
      else result += c;
    } else if (in_double) {
      if (c == '"') in_double = false;
      else if (c == '\\' && i + 1 < token.length()) {
        result += token[i+1];
        i++;
      } else {
        result += c;
      }
    } else {
      if (c == '\'') in_single = true;
      else if (c == '"') in_double = true;
      else if (c == '\\' && i + 1 < token.length()) {
        result += token[i+1];
        i++;
      } else {
        result += c;
      }
    }
  }
  return result;
}

void splitPath(const std::string& path, std::string& dir, std::string& prefix) {
  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos) {
    dir = ".";
    prefix = path;
  } else if (last_slash == 0) {
    dir = "/";
    prefix = path.substr(1);
  } else {
    dir = path.substr(0, last_slash);
    prefix = path.substr(last_slash + 1);
  }
}

std::vector<std::string> getMatchingPaths(const std::string& active_token) {
  using namespace std;
  vector<string> matches;
  string clean_token = cleanPathToken(active_token);
  
  string dir, prefix;
  splitPath(clean_token, dir, prefix);
  
  try {
    if (filesystem::exists(dir) && filesystem::is_directory(dir)) {
      for (const auto& entry : filesystem::directory_iterator(dir)) {
        string filename = entry.path().filename().string();
        if (filename.rfind(".", 0) == 0 && prefix.rfind(".", 0) != 0) {
          continue;
        }
        if (filename.rfind(prefix, 0) == 0) {
          string match_path = "";
          if (clean_token.find_last_of('/') != string::npos) {
            match_path = clean_token.substr(0, clean_token.find_last_of('/') + 1) + filename;
          } else {
            match_path = filename;
          }
          
          if (entry.is_directory()) {
            match_path += "/";
          }
          matches.push_back(match_path);
        }
      }
    }
  } catch (...) {
  }
  
  sort(matches.begin(), matches.end());
  return matches;
}

std::string readInput() {
  using namespace std;
  cout << SHELL_PROMPT << flush;
  
  string command = "";
  enableRawMode();
  
  int last_tab_count = 0;
  
  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) { // reads a character from the standard input, 0 if EOF, -1 on error
      break;
    }
    
    if (c == '\n' || c == '\r') { // newline or carriage return
      cout << endl;
      break;
    } else if (c == 127 || c == 8) { // backspace or delete
      last_tab_count = 0;
      if (!command.empty()) { // check command because command could be empty and it would lead to segmentation fault
        command.pop_back(); // remove last character
        cout << "\b \b" << flush; // 1. move cursor back, 2. print space, 3. move cursor back again
      }
    } else if (c == '\t') { // tab
      last_tab_count++;
      TokenInfo target = parseCompletionTarget(command);
      vector<string> matches;
      if (target.is_command) {
        matches = getMatchingCommands(target.active_token);
      } else {
        matches = getMatchingPaths(target.active_token);
      }

      if (matches.empty()) {
        cout << "\a" << flush; // beep
        last_tab_count = 0;
      } else {
        string lcp = getLongestCommonPrefix(matches);
        if (lcp.length() > target.active_token.length()) {
          string suffix = lcp.substr(target.active_token.length());
          if (matches.size() == 1 && !matches[0].empty() && matches[0].back() != '/') {
            suffix += " ";
            command = target.base_prefix + lcp + " ";
          } else {
            command = target.base_prefix + lcp;
          }
          cout << suffix << flush;
          last_tab_count = 0;
        } else {
          if (matches.size() == 1) {
            if (target.active_token.empty() || (target.active_token.back() != ' ' && matches[0].back() != '/')) {
              if (matches[0].back() != '/') {
                cout << " " << flush;
                command += " ";
              }
            }
            last_tab_count = 0;
          } else {
            if (last_tab_count == 1) {
              cout << "\a" << flush;
            } else if (last_tab_count == 2) {
              cout << "\n";
              for (size_t i = 0; i < matches.size(); ++i) {
                if (i > 0) cout << "  ";
                string display_name = matches[i];
                if (!target.is_command) {
                  size_t slash_pos = matches[i].find_last_of('/', matches[i].length() - 2);
                  if (slash_pos != string::npos) {
                    display_name = matches[i].substr(slash_pos + 1);
                  }
                }
                cout << display_name;
              }
              cout << "\n" << SHELL_PROMPT << command << flush;
              last_tab_count = 0;
            }
          }
        }
      }
    } else if (isprint(c)) { // printable character
      last_tab_count = 0;
      command += c;
      cout << c << flush;
    }
  }
  
  disableRawMode(); // to restore the terminal to its original state
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


std::vector<std::string> parseArguments(std::string message) {
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
  return args;
}

std::string cleanInput(std::string message) {
  using namespace std;
  vector<string> args = parseArguments(message);
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

  vector<string> tokens = parseArguments(command);
  if (tokens.empty()) {
    return "";
  }

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

  shellLoop();
}
