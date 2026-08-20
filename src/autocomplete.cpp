#include "autocomplete.h"
#include "commands.h"
#include "parser.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <unistd.h>

vector<string> getMatchingCommands(const string &prefix) {
  set<string> unique_matches;
  vector<string> builtins = BUILTINS;

  for (const string &b : builtins) {
    if (b.rfind(prefix, 0) == 0) {
      unique_matches.insert(b);
    }
  }

  char *path = getenv("PATH"); // PATH is a colon separated list of directories
  if (path) {
    string pathStr = path; // convert to string
    // Split the path string into a vector of strings based on the delimiter ':'
    vector<string> path_dirs = stringSplit(pathStr, ':');
    for (const string &dir : path_dirs) {
      // Iterate over the directory entries
      for (const filesystem::directory_entry &entry :
           filesystem::directory_iterator(dir)) {
        string filename = entry.path().filename().string();
        if (filename.rfind(prefix, 0) == 0) {
          if (access(entry.path().c_str(), X_OK) == 0) {
            unique_matches.insert(filename);
          }
        }
      }
    }
  }
  vector<string> matching_commands(unique_matches.begin(),
                                   unique_matches.end());
  sort(matching_commands.begin(), matching_commands.end());
  return matching_commands;
}

string getLongestCommonPrefix(const vector<string> &matches) {
  if (matches.empty())
    return "";
  string prefix = matches[0];
  for (size_t i = 1; i < matches.size(); ++i) {
    while (matches[i].find(prefix) != 0) {
      prefix = prefix.substr(0, prefix.length() - 1);
      if (prefix.empty())
        return "";
    }
  }
  return prefix;
}

// This function gets the matching paths
vector<string> getMatchingPaths(const string &active_token) {
  vector<string> matches;
  string clean_token = cleanPathToken(active_token);

  string dir, prefix;
  splitPath(clean_token, dir, prefix);

  try {
    if (filesystem::exists(dir) && filesystem::is_directory(dir)) {
      for (const auto &entry : filesystem::directory_iterator(dir)) {
        string filename = entry.path().filename().string();
        // Skip hidden files unless the prefix is also hidden
        if (filename.rfind(".", 0) == 0 && prefix.rfind(".", 0) != 0) {
          continue;
        }
        // If the filename starts with the prefix, it is a match
        if (filename.rfind(prefix, 0) == 0) {
          string match_path = "";
          if (clean_token.find_last_of('/') != string::npos) {
            match_path =
                clean_token.substr(0, clean_token.find_last_of('/') + 1) +
                filename;
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

vector<string> runCompleter(const string &completer_path,
                            const string &cmd_name, const string &current_word,
                            const string &previous_word,
                            const string &full_line) {
  setenv("COMP_LINE", full_line.c_str(), 1); // comp_line is the full command line
  setenv("COMP_POINT", to_string(current_word.length()).c_str(), 1); // comp_point is the position of the cursor
  setenv("COMP_KEY", "9", 1); // comp_key is the key that was pressed to trigger the completion
  setenv("COMP_TYPE", "9", 1); // comp_type is the type of completion, eg: 1 = normal completion, 2 = menu completion, etc.
  // comp_words is the number of words in the command
  vector<string> cmd_tokens = parseArguments(full_line); 
  setenv("COMP_WORDS", to_string(cmd_tokens.size()).c_str(), 1);
  // comp_word is the current word being completed
  setenv("COMP_WORD", current_word.c_str(), 1);
  // comp_prev_word is the previous word
  // comp_dir is the directory of the current word
  string comp_dir = current_word.substr(0, current_word.find_last_of('/'));
  setenv("COMP_DIR", comp_dir.c_str(), 1);
  // comp_prefix is the prefix of the current word
  string comp_prefix = current_word.substr(current_word.find_last_of('/') + 1);
  setenv("COMP_PREFIX", comp_prefix.c_str(), 1);
  
  setenv("COMP_PREV_WORD", previous_word.c_str(), 1);

  string exec_cmd = completer_path + " '" + cmd_name + "' '" + current_word +
                    "' '" + previous_word + "'";

  vector<string> results;
  FILE *pipe = popen(exec_cmd.c_str(), "r");
  if (!pipe)
    return results;

  char buffer[128];
  string line = "";
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) { // reading the output of the completer
    line += buffer;
    size_t pos;
    // Get all the candidates separated by new lines 
    while ((pos = line.find('\n')) != string::npos) { // new line character
      string cand = line.substr(0, pos); // candidate
      if (!cand.empty() && cand.back() == '\r') { // carriage return character
        cand.pop_back();
      }
      results.push_back(cand); // adding the candidate to the results
      line.erase(0, pos + 1); // removing the new line character
    }
  }
  if (!line.empty()) { // adding the last candidate to the results
    results.push_back(line);
  }
  pclose(pipe); // closing the pipe
  return results;
}