#include "commands.h"
#include "parser.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

string pwd() {
  filesystem::path cwd = filesystem::current_path();
  return cwd.string();
}

string cd(const string &directory) {
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

string findInPATH(const string &command) {
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

string typeCommand(const string &command) {
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

vector<Job> background_jobs;
vector<string> command_history;

void reapJobs() {
  for (auto &job : background_jobs) {
    if (job.status == "Running") {
      int status;
      pid_t result = waitpid(job.pid, &status, WNOHANG);
      if (result > 0 || result == -1) {
        job.status = "Done";
      }
    }
  }
}

void printAndClearCompletedJobs() {
  reapJobs();
  for (auto it = background_jobs.begin(); it != background_jobs.end();) {
    if (it->status == "Done") {
      cout << "[" << it->job_number << "]+  Done                 " << it->command << endl;
      it = background_jobs.erase(it);
    } else {
      it++;
    }
  }
}

string execute(const string &command) {
  string cmd = command;
  vector<string> tokens = parseArguments(cmd);
  if (tokens.empty()) {
    return "";
  }

  bool run_in_background = false;
  if (tokens.back() == "&") {
    run_in_background = true;
    tokens.pop_back();
  }

  if (tokens.empty()) {
    return "";
  }

  string job_command = "";
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (i > 0) job_command += " ";
    job_command += tokens[i];
  }

  // converting vector of string to array of char*
  vector<char *> args;
  // char **args = new char *[tokens.size() + 1];
  for (size_t i = 0; i < tokens.size(); i++) {
    args.push_back(const_cast<char *>(tokens[i].c_str()));
  }
  args.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args.data());
    // If execvp returns, it failed
    cerr << args[0] << ": command not found" << endl;
    exit(127);
  } else if (pid < 0) {
    return "Error in forking";
  } else {
    if (run_in_background) {
      int job_num = 1;
      if (!background_jobs.empty()) {
        int max_num = 0;
        for (const auto &job : background_jobs) {
          if (job.job_number > max_num) {
            max_num = job.job_number;
          }
        }
        job_num = max_num + 1;
      }
      background_jobs.push_back({job_num, pid, job_command, "Running"});
      cout << "[" << job_num << "] " << pid << endl;
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
  return "";
}
string echo(const string &message) { return cleanInput(message); }

// Map to store command completion specifications
unordered_map<string, string> completion_specs;

string completeCommand(const string &command) {
  vector<string> tokens = parseArguments(command);
  if (tokens.empty())
    return "";

  if (tokens[0] == "-p") {
    if (tokens.size() < 2) {
      return "complete: usage: complete -p command";
    }
    string cmd_name = tokens[1];
    if (completion_specs.count(cmd_name)) {
      return "complete -C '" + completion_specs[cmd_name] + "' " + cmd_name;
    } else {
      return "complete: " + cmd_name + ": no completion specification";
    }
  } else if (tokens[0] == "-C") {
    if (tokens.size() < 3) {
      return "complete: usage: complete -C completer command";
    }
    string completer = tokens[1];
    string cmd_name = tokens[2];
    completion_specs[cmd_name] = completer;
    return "";
  } else if (tokens[0] == "-r") {
    if (tokens.size() < 2) {
      return "complete: usage: complete -r command";
    }
    string cmd_name = tokens[1];
    if (completion_specs.count(cmd_name)) {
      completion_specs.erase(cmd_name);
      return "";
    } else {
      return "complete: " + cmd_name + ": no completion specification";
    }
  }

  return "complete: unsupported option";
}

string evaluateCommand(const string &command) {
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
  } else if (command.substr(0, 8) == "complete" && command[8] == ' ') {
    return completeCommand(command.substr(9));
  } else if (command.substr(0, 4) == "jobs") {
    reapJobs();
    string out = "";
    size_t num_jobs = background_jobs.size();
    for (size_t i = 0; i < num_jobs; ++i) {
      const auto &job = background_jobs[i];
      string marker = " ";
      if (i == num_jobs - 1) {
        marker = "+";
      } else if (i == num_jobs - 2) {
        marker = "-";
      }
      string extra = (job.status == "Running") ? " &" : "";
      out += "[" + to_string(job.job_number) + "]" + marker + "  " + job.status + "                 " + job.command + extra + "\n";
    }
    for (auto it = background_jobs.begin(); it != background_jobs.end();) {
      if (it->status == "Done") {
        it = background_jobs.erase(it);
      } else {
        it++;
      }
    }
    if (!out.empty() && out.back() == '\n') {
      out.pop_back();
    }
    return out;
  } else if (command == "history" || (command.length() >= 8 && command.substr(0, 7) == "history" && command[7] == ' ')) {
    vector<string> args = parseArguments(command);
    if (args.size() > 1 && args[1] == "-r") {
      if (args.size() > 2) {
        ifstream file(args[2]);
        if (file.is_open()) {
          string line;
          while (getline(file, line)) {
            if (!line.empty() && line.back() == '\r') {
              line.pop_back();
            }
            if (!line.empty()) {
              command_history.push_back(line);
            }
          }
        }
      }
      return "";
    }
    size_t n = command_history.size();
    if (args.size() > 1) {
      try {
        int count = stoi(args[1]);
        if (count >= 0) {
          n = static_cast<size_t>(count);
        }
      } catch (...) {
        // Fall back to full history if parsing fails
      }
    }
    size_t start_idx = (command_history.size() > n) ? (command_history.size() - n) : 0;
    stringstream ss;
    for (size_t i = start_idx; i < command_history.size(); ++i) {
      ss << setw(5) << (i + 1) << "  " << command_history[i] << "\n";
    }
    string out = ss.str();
    if (!out.empty() && out.back() == '\n') {
      out.pop_back();
    }
    return out;
  } else {
    return execute(command);
  }
}

void printOutput(const string &output) {
  if (!output.empty()) {
    cout << output << endl;
  }
}