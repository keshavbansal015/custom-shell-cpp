#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>

struct TokenInfo {
  std::string base_prefix;
  std::string active_token;
  bool is_command;
};

std::vector<std::string> stringSplit(const std::string &str, char delimiter);
TokenInfo parseCompletionTarget(const std::string &command);
std::string cleanPathToken(const std::string &token);
void splitPath(const std::string &path, std::string &dir, std::string &prefix);
std::vector<std::string> parseArguments(std::string &message);
std::string cleanInput(std::string &message);

#endif
