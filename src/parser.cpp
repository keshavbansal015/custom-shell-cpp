#include "parser.h"
#include <sstream>

vector<string> stringSplit(const string &str, char delimiter) {
  vector<string> tokens;
  string token;
  istringstream tokenStream(str);
  while (getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

// This function parses the command line to determine what needs to be completed
TokenInfo parseCompletionTarget(const string &command) {
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
      } else if (c == '\\' && i + 1 < command.length()) { // escaped character
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

  if (last_was_space) {
    info.base_prefix = command;
    info.active_token = "";
  } else {
    info.base_prefix = command.substr(0, last_token_start);
    info.active_token = command.substr(last_token_start);
  }

  bool has_non_space_before = false;
  for (char c : info.base_prefix) { 
    if (c != ' ') {
      has_non_space_before = true;
      break;
    }
  }

  // active token contains a slash so it is a path completion
  if (has_non_space_before ||
      info.active_token.find('/') != string::npos) {
    info.is_command = false;
  }

  return info;
}

// This function cleans the path token by removing quotes and escape characters
// example: "dir/" -> "dir/", "'dir'" -> "dir", "\"dir\"" -> "dir", "" -> ""
string cleanPathToken(const string &token) {
  string result = "";
  bool in_single = false;
  bool in_double = false;
  for (size_t i = 0; i < token.length(); ++i) {
    char c = token[i];
    if (in_single) {
      if (c == '\'')
        in_single = false;
      else
        result += c;
    } else if (in_double) {
      if (c == '"')
        in_double = false;
      else if (c == '\\' && i + 1 < token.length()) {
        result += token[i + 1];
        i++;
      } else {
        result += c;
      }
    } else {
      if (c == '\'')
        in_single = true;
      else if (c == '"')
        in_double = true;
      else if (c == '\\' && i + 1 < token.length()) {
        result += token[i + 1];
        i++;
      } else {
        result += c;
      }
    }
  }
  return result;
}

// This function splits a path into a directory and a prefix
void splitPath(const string &path, string &dir, string &prefix) {
  size_t last_slash = path.find_last_of('/');
  if (last_slash ==
      string::npos) { // example: path = "dir", dir = ".", prefix = "dir"
    dir = ".";
    prefix = path;
  } else if (last_slash ==
             0) { // example: path = "/dir", dir = "/", prefix = "dir"
    dir = "/";
    prefix = path.substr(1);
  } else { // example: path = "dir/dir", dir = "dir", prefix = "dir"
    dir = path.substr(0, last_slash);
    prefix = path.substr(last_slash + 1);
  }
}

// This function parses the command line and returns a vector of arguments
// example: "echo \"Hello World\"" -> ["echo", "Hello World"]
// example: "cd /usr/bin/" -> ["cd", "/usr/bin/"]
vector<string> parseArguments(const string &message) {
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
      } else if (c == '\\' && i + 1 < message.length()) { // example: "hello\n" -> "hello\n"
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

string cleanInput(const string &message) {
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

CommandRedirection parseRedirection(const string &command) {
  CommandRedirection result;
  result.clean_command = "";
  result.redirect_file = "";
  result.has_redirection = false;

  bool in_single_quotes = false;
  bool in_double_quotes = false;

  size_t i = 0;
  size_t n = command.length();

  while (i < n) {
    char c = command[i];
    if (in_single_quotes) {
      if (c == '\'') {
        in_single_quotes = false;
      }
      result.clean_command += c;
      i++;
    } else if (in_double_quotes) {
      if (c == '"') {
        in_double_quotes = false;
      } else if (c == '\\' && i + 1 < n) {
        result.clean_command += c;
        result.clean_command += command[i + 1];
        i += 2;
        continue;
      }
      result.clean_command += c;
      i++;
    } else {
      if (c == '\'') {
        in_single_quotes = true;
        result.clean_command += c;
        i++;
      } else if (c == '"') {
        in_double_quotes = true;
        result.clean_command += c;
        i++;
      } else if (c == '\\' && i + 1 < n) {
        result.clean_command += c;
        result.clean_command += command[i + 1];
        i += 2;
      } else if (c == '>') {
        result.has_redirection = true;
        i++; // skip '>'
        
        while (i < n && command[i] == ' ') {
          i++;
        }
        
        string file_token = "";
        bool file_in_single = false;
        bool file_in_double = false;
        while (i < n) {
          char fc = command[i];
          if (file_in_single) {
            if (fc == '\'') {
              file_in_single = false;
            } else {
              file_token += fc;
            }
            i++;
          } else if (file_in_double) {
            if (fc == '"') {
              file_in_double = false;
            } else if (fc == '\\' && i + 1 < n) {
              file_token += command[i + 1];
              i += 2;
            } else {
              file_token += fc;
            }
            i++;
          } else {
            if (fc == '\'') {
              file_in_single = true;
              i++;
            } else if (fc == '"') {
              file_in_double = true;
              i++;
            } else if (fc == '\\' && i + 1 < n) {
              file_token += command[i + 1];
              i += 2;
            } else if (fc == ' ') {
              break;
            } else if (fc == '>') {
              break;
            } else {
              file_token += fc;
              i++;
            }
          }
        }
        result.redirect_file = file_token;
      } else if (c == '1' && i + 1 < n && command[i + 1] == '>') {
        result.has_redirection = true;
        i += 2; // skip '1>'
        
        while (i < n && command[i] == ' ') {
          i++;
        }
        
        string file_token = "";
        bool file_in_single = false;
        bool file_in_double = false;
        while (i < n) {
          char fc = command[i];
          if (file_in_single) {
            if (fc == '\'') {
              file_in_single = false;
            } else {
              file_token += fc;
            }
            i++;
          } else if (file_in_double) {
            if (fc == '"') {
              file_in_double = false;
            } else if (fc == '\\' && i + 1 < n) {
              file_token += command[i + 1];
              i += 2;
            } else {
              file_token += fc;
            }
            i++;
          } else {
            if (fc == '\'') {
              file_in_single = true;
              i++;
            } else if (fc == '"') {
              file_in_double = true;
              i++;
            } else if (fc == '\\' && i + 1 < n) {
              file_token += command[i + 1];
              i += 2;
            } else if (fc == ' ') {
              break;
            } else if (fc == '>') {
              break;
            } else {
              file_token += fc;
              i++;
            }
          }
        }
        result.redirect_file = file_token;
      } else {
        result.clean_command += c;
        i++;
      }
    }
  }

  while (!result.clean_command.empty() && result.clean_command.back() == ' ') {
    result.clean_command.pop_back();
  }

  return result;
}