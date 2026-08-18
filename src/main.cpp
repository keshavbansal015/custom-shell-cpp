#include "autocomplete.h"
#include "commands.h"
#include "parser.h"
#include "terminal.h"
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define SHELL_PROMPT "$ "

string readInput() {
  using namespace std;
  cout << SHELL_PROMPT << flush;

  string command = "";
  enableRawMode();

  int last_tab_count = 0;

  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) <=
        0) { // reads a character from the standard input, 0 if EOF, -1 on error
      break;
    }

    if (c == '\n' || c == '\r') { // newline or carriage return
      cout << endl;
      break;
    } else if (c == 127 || c == 8) { // backspace or delete
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
          if (matches.size() == 1 && !matches[0].empty() &&
              matches[0].back() != '/') {
            suffix += " ";
            command = target.base_prefix + lcp + " ";
          } else {
            command = target.base_prefix + lcp;
          }
          cout << suffix << flush;
          last_tab_count = 0;
        } else {
          if (matches.size() == 1) {
            if (target.active_token.empty() ||
                (target.active_token.back() != ' ' &&
                 matches[0].back() != '/')) {
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
                if (i > 0)
                  cout << "  ";
                string display_name = matches[i];
                if (!target.is_command) {
                  size_t slash_pos =
                      matches[i].find_last_of('/', matches[i].length() - 2);
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

void shellLoop() {
  using namespace std;
  while (true) {
    string command = readInput();
    string output = evaluateCommand(command);
    printOutput(output);
  }
}

int main() {
  // Flush after every cout / std:cerr
  cout << unitbuf;
  cerr << unitbuf;

  shellLoop();
}
