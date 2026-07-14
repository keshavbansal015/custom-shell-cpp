#include "shell.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <ranges>
#include <filesystem>
#include <cstdlib>

Shell::Shell() : m_builtins({"echo", "type", "pwd", "cd", "exit"}), m_prompt("$ ") {}

void Shell::run() {
    while (true) {
        std::string command = readInput();
        std::string output = evaluateCommand(command);
        printOutput(output);
    }
}

std::string Shell::readInput() const {
    std::cout << m_prompt;
    std::string command;
    std::getline(std::cin, command);
    return command;
}

void Shell::printOutput(std::string_view output) const {
    if (!output.empty()) {
        std::cout << output << std::endl;
    }
}

bool Shell::isBuiltin(std::string_view command, std::string& matchedBuiltin) const {
    for (const auto& builtin : m_builtins) {
        if (command.substr(0, builtin.length()) == builtin) {
            matchedBuiltin = builtin;
            return true;
        }
    }
    return false;
}

std::string Shell::findInPath(std::string_view command) const {
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) {
        return "";
    }
    
    std::string_view pathStr(pathEnv);
    // Split PATH by ':' using ranges to optimize
    for (const auto dirRange : pathStr | std::views::split(':')) {
        std::string currDir{dirRange.begin(), dirRange.end()};
        std::string fullPath = currDir + "/" + std::string(command);
        if (access(fullPath.c_str(), F_OK) == 0 && access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }
    return "";
}

std::string Shell::handleEcho(std::string_view args) {
    return std::string(args);
}

std::string Shell::handleType(std::string_view args) {
    std::string matched;
    if (isBuiltin(args, matched)) {
        return matched + " is a shell builtin";
    }

    std::string pathLoc = findInPath(args);
    if (pathLoc.empty()) {
        return std::string(args) + ": not found";
    }
    return std::string(args) + " is " + pathLoc;
}

std::string Shell::handlePwd() {
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    if (ec) {
        return "";
    }
    return cwd.string();
}

std::string Shell::handleCd(std::string_view directory) {
    if (directory.empty()) {
        return "";
    }

    std::string targetDir = std::string(directory);
    if (directory == "~") {
        const char* home = std::getenv("HOME");
        targetDir = home ? home : "";
    } else if (directory == "-") {
        const char* oldpwd = std::getenv("OLDPWD");
        if (!oldpwd) {
            return "cd: OLDPWD not set";
        }
        targetDir = oldpwd;
    }

    std::error_code ec;
    std::string oldPath = std::filesystem::current_path(ec).string();

    if (chdir(targetDir.c_str()) == 0) {
        setenv("OLDPWD", oldPath.c_str(), 1);
        return "";
    }

    return "cd: " + std::string(directory) + ": No such file or directory";
}

std::string Shell::handleExternal(std::string_view command) {
    std::vector<std::string> tokens = command 
                                    | std::views::split(' ') 
                                    | std::ranges::to<std::vector<std::string>>();

    if (tokens.empty()) {
        return "";
    }

    // Convert vector of string to vector of char* to avoid manual raw allocation/deletion
    std::vector<char*> args;
    args.reserve(tokens.size() + 1);
    for (auto& token : tokens) {
        args.push_back(const_cast<char*>(token.c_str()));
    }
    args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args.data());
        // If execvp returns, it failed
        std::cerr << args[0] << ": command not found" << std::endl;
        std::exit(127);
    } else if (pid < 0) {
        return "Error in forking";
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
    return "";
}

std::string Shell::evaluateCommand(std::string_view command) {
    if (command == "exit") {
        std::exit(0);
    } else if (command.length() >= 5 && command.substr(0, 5) == "echo ") {
        return handleEcho(command.substr(5));
    } else if (command.length() >= 5 && command.substr(0, 5) == "type ") {
        return handleType(command.substr(5));
    } else if (command == "pwd") {
        return handlePwd();
    } else if (command.length() >= 3 && command.substr(0, 3) == "cd ") {
        return handleCd(command.substr(3));
    } else {
        return handleExternal(command);
    }
}
