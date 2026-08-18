#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H
#include <string>
#include <vector>
std::vector<std::string> getMatchingCommands(const std::string &prefix);
std::string getLongestCommonPrefix(const std::vector<std::string> &matches);
std::vector<std::string> getMatchingPaths(const std::string &active_token);
#endif