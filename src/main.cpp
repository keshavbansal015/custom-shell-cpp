#include "autocomplete.h"
#include "commands.h"
#include "parser.h"
#include "terminal.h"
#include <cctype>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define SHELL_PROMPT "$ "

string readInput() {
  using namespace std;
  printAndClearCompletedJobs();
  cout << SHELL_PROMPT << flush;

  string command = "";
  enableRawMode();

  int last_tab_count = 0;

  while (true) {
    char c;
    // reads a character from the standard input, 0 if EOF, -1 on error
    if (read(STDIN_FILENO, &c, 1) <= 0) {
      break;
    }

    if (c == '\n' || c == '\r') { 
      // newline or carriage return
      cout << endl;
      break;
    } else if (c == 127 || c == 8) { 
      // backspace or delete
      last_tab_count = 0;
      if (!command.empty()) { // check command because command could be empty
                              // and it would lead to segmentation fault
        command.pop_back();   // remove last character
        cout << "\b \b" << flush; // 1. move cursor back, 2. print space, 3.
                                  // move cursor back again
      }
    } else if (c == '\t') { // tab
      last_tab_count++;
      TokenInfo target = parseCompletionTarget(command); 
      vector<string> cmd_tokens = parseArguments(command);
      bool ran_programmable = false; // If a programmable completer was run
      // Check if there is a programmable completer for the command
      if (!cmd_tokens.empty() && completion_specs.count(cmd_tokens[0])) {
        string cmd_name = cmd_tokens[0];
        string current_word = target.active_token;
        string previous_word = "";
        // If active token is empty, previous word is the last token in the command
        // Otherwise, previous word is the second to last token in the command
        // example: command = "cd /usr/bin/", active_token = "", cmd_tokens = ["cd", "/usr/bin/"], previous_word = "/usr/bin/"
        // example: command = "cd /usr/bin/a", active_token = "a", cmd_tokens = ["cd", "/usr/bin/a"], previous_word = "/usr/bin/a"
        if (target.active_token.empty()) {
          previous_word = cmd_tokens.back();
        } else {
          if (cmd_tokens.size() > 1) {
            previous_word = cmd_tokens[cmd_tokens.size() - 2];
          }
        }
        // Run the programmable completer
        // example: command = "cd /usr/bin/", cmd_name = "cd", current_word = "", previous_word = "/usr/bin/"
        // result: ["bin", "boot", "dev", ...]
        // command = "ls /usr/bin/", cmd_name = "ls", current_word = "", previous_word = "/usr/bin/"
        // result: ["/", "/bin", "/boot", ...]
        // command = "git checkout " , cmd_name = "git", current_word = "", previous_word = "git checkout "
        // result: ["branch", "tag", ...]
        // command = "git ", cmd_name = "git", current_word = "", previous_word = "git "
        // result: ["branch", "tag", ...]
        cmd_tokens = runCompleter(completion_specs[cmd_name], cmd_name,
                                  current_word, previous_word, command);
        ran_programmable = true;
      }

      if (!ran_programmable) {
        if (target.is_command) {
          cmd_tokens = getMatchingCommands(target.active_token);
        } else {
          cmd_tokens = getMatchingPaths(target.active_token);
        }
      }

      if (cmd_tokens.empty()) {
        cout << "\a" << flush; // beep
        last_tab_count = 0;
      } else {
        string lcp = getLongestCommonPrefix(cmd_tokens);
        if (lcp.length() > target.active_token.length()) {
          string suffix = lcp.substr(target.active_token.length());
          if (cmd_tokens.size() == 1 && !cmd_tokens[0].empty() &&
              cmd_tokens[0].back() != '/') {
            suffix += " ";
            command = target.base_prefix + lcp + " ";
          } else {
            command = target.base_prefix + lcp;
          }
          cout << suffix << flush;
          last_tab_count = 0;
        } else {
          if (cmd_tokens.size() == 1) {
            if (target.active_token.empty() ||
                (target.active_token.back() != ' ' &&
                 cmd_tokens[0].back() != '/')) {
              if (cmd_tokens[0].back() != '/') {
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
              for (size_t i = 0; i < cmd_tokens.size(); ++i) {
                if (i > 0)
                  cout << "  ";
                string display_name = cmd_tokens[i];
                if (!target.is_command) {
                  size_t slash_pos = cmd_tokens[i].find_last_of(
                      '/', cmd_tokens[i].length() - 2);
                  if (slash_pos != string::npos) {
                    display_name = cmd_tokens[i].substr(slash_pos + 1);
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

void shellLoop() {
  using namespace std;
  while (true) {
    string command = readInput();
    CommandRedirection redirect = parseRedirection(command);

    int saved_stdout = -1;
    int file_fd = -1;
    if (redirect.has_redirection) {
      // Create directories if they don't exist
      filesystem::path p(redirect.redirect_file);
      if (p.has_parent_path()) {
        filesystem::create_directories(p.parent_path());
      }
      
      file_fd = open(redirect.redirect_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (file_fd >= 0) {
        saved_stdout = dup(STDOUT_FILENO);
        dup2(file_fd, STDOUT_FILENO);
        close(file_fd);
      }
    }

    string output = evaluateCommand(redirect.clean_command);
    printOutput(output);

    if (saved_stdout >= 0) {
      dup2(saved_stdout, STDOUT_FILENO);
      close(saved_stdout);
    }
  }
}

int main() {
  // Flush after every cout / std:cerr
  cout << unitbuf;
  cerr << unitbuf;

  shellLoop();
}
