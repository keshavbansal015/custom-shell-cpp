#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string_view>

class Shell {
public:
    Shell();
    
    // Starts the shell loop
    void run();

private:
    // Helper to read user input
    std::string readInput() const;

    // Helper to print output
    void printOutput(std::string_view output) const;

    // Dispatcher for commands
    std::string evaluateCommand(std::string_view command);

    // Command implementations matching original behavior
    std::string handleEcho(std::string_view args);
    std::string handleType(std::string_view args);
    std::string handlePwd();
    std::string handleCd(std::string_view directory);
    std::string handleExternal(std::string_view command);

    // Helpers
    std::string findInPath(std::string_view command) const;
    bool isBuiltin(std::string_view command, std::string& matchedBuiltin) const;

    std::vector<std::string> m_builtins;
    std::string m_prompt;
};
