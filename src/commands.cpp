#include "commands.h"
#include "parser.h"
#include <fcntl.h>
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
  vector<string> tokens = parseArguments(directory);
  if (tokens.empty()) {
    return "";
  }

  string targetDir = tokens[0];
  if (targetDir == "~") {
    char *home = getenv("HOME");
    targetDir = home ? home : "";
  } else if (targetDir == "-") {
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

  return "cd: " + targetDir + ": No such file or directory";
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
  vector<string> tokens = parseArguments(command);
  if (tokens.empty()) {
    return "";
  }
  string cmd = tokens[0];
  // check if the command is a shell builtin
  for (string shellCommand : BUILTINS) {
    if (cmd == shellCommand) {
      return shellCommand + " is a shell builtin";
    }
  }

  string output = findInPATH(cmd);
  if (output == "") {
    return cmd + ": not found";
  }
  return cmd + " is " + output;
}

vector<Job> background_jobs;
vector<string> command_history;
size_t history_appended_offset = 0;
unordered_map<string, string> shell_variables;

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
          history_appended_offset = command_history.size();
        }
      }
      return "";
    } else if (args.size() > 1 && args[1] == "-w") {
      if (args.size() > 2) {
        filesystem::path p(args[2]);
        if (p.has_parent_path()) {
          filesystem::create_directories(p.parent_path());
        }
        ofstream file(args[2]);
        if (file.is_open()) {
          for (const auto &cmd : command_history) {
            file << cmd << "\n";
          }
        }
      }
      return "";
    } else if (args.size() > 1 && args[1] == "-a") {
      if (args.size() > 2) {
        filesystem::path p(args[2]);
        if (p.has_parent_path()) {
          filesystem::create_directories(p.parent_path());
        }
        ofstream file(args[2], ios::app);
        if (file.is_open()) {
          for (size_t i = history_appended_offset; i < command_history.size(); ++i) {
            file << command_history[i] << "\n";
          }
          history_appended_offset = command_history.size();
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
  } else if (command == "declare" || (command.length() >= 8 && command.substr(0, 7) == "declare" && command[7] == ' ')) {
    return declareCommand(command.length() >= 8 ? command.substr(8) : "");
  } else {
    return execute(command);
  }
}

void printOutput(const string &output) {
  if (!output.empty()) {
    cout << output << endl;
  }
}

void loadHistoryFromFile() {
  const char *histfile = getenv("HISTFILE");
  if (histfile && string(histfile).length() > 0) {
    ifstream file(histfile);
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
      history_appended_offset = command_history.size();
    }
  }
}

void saveHistoryToFile() {
  const char *histfile = getenv("HISTFILE");
  if (histfile && string(histfile).length() > 0) {
    filesystem::path p(histfile);
    if (p.has_parent_path()) {
      filesystem::create_directories(p.parent_path());
    }
    ofstream file(histfile);
    if (file.is_open()) {
      for (const auto &cmd : command_history) {
        file << cmd << "\n";
      }
    }
  }
}

bool isValidIdentifier(const string &name) {
  if (name.empty()) return false;
  if (!isalpha(name[0]) && name[0] != '_') return false;
  for (size_t i = 1; i < name.length(); ++i) {
    if (!isalnum(name[i]) && name[i] != '_') return false;
  }
  return true;
}

string declareCommand(const string &message) {
  vector<string> tokens = parseArguments(message);
  if (tokens.empty()) {
    return "";
  }

  if (tokens[0] == "-p") {
    if (tokens.size() == 1) {
      string out = "";
      for (const auto &pair : shell_variables) {
        if (!out.empty()) out += "\n";
        out += "declare -- " + pair.first + "=\"" + pair.second + "\"";
      }
      return out;
    }
    string out = "";
    for (size_t i = 1; i < tokens.size(); ++i) {
      const string &var_name = tokens[i];
      if (shell_variables.count(var_name)) {
        if (!out.empty()) out += "\n";
        out += "declare -- " + var_name + "=\"" + shell_variables[var_name] + "\"";
      } else {
        if (!out.empty()) out += "\n";
        out += "declare: " + var_name + ": not found";
      }
    }
    return out;
  } else if (tokens[0] == "-F") {
    return "";
  } else {
    string out = "";
    for (size_t i = 0; i < tokens.size(); ++i) {
      const string &tok = tokens[i];
      size_t eq_pos = tok.find('=');
      string var_name = (eq_pos != string::npos) ? tok.substr(0, eq_pos) : tok;
      string var_val = (eq_pos != string::npos) ? tok.substr(eq_pos + 1) : "";

      if (!isValidIdentifier(var_name)) {
        if (!out.empty()) out += "\n";
        out += "declare: `" + tok + "': not a valid identifier";
      } else {
        if (eq_pos != string::npos) {
          shell_variables[var_name] = var_val;
        } else {
          if (!shell_variables.count(var_name)) {
            shell_variables[var_name] = "";
          }
        }
      }
    }
    return out;
  }
}

void executePipeline(const vector<string> &stages) {
  int num_stages = stages.size();
  int prev_fd = -1;
  vector<pid_t> pids;

  for (int i = 0; i < num_stages; ++i) {
    int pipefd[2];
    if (i < num_stages - 1) {
      if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
      }
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return;
    }

    if (pid == 0) { // Child process
      if (i > 0) {
        dup2(prev_fd, STDIN_FILENO);
        close(prev_fd);
      }
      if (i < num_stages - 1) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
      }

      CommandRedirection redirect = parseRedirection(stages[i]);
      if (redirect.redirect_stdout) {
        filesystem::path p(redirect.stdout_file);
        if (p.has_parent_path()) {
          filesystem::create_directories(p.parent_path());
        }
        int flags = O_WRONLY | O_CREAT | (redirect.append_stdout ? O_APPEND : O_TRUNC);
        int out_fd = open(redirect.stdout_file.c_str(), flags, 0644);
        if (out_fd >= 0) {
          dup2(out_fd, STDOUT_FILENO);
          close(out_fd);
        }
      }
      if (redirect.redirect_stderr) {
        filesystem::path p(redirect.stderr_file);
        if (p.has_parent_path()) {
          filesystem::create_directories(p.parent_path());
        }
        int flags = O_WRONLY | O_CREAT | (redirect.append_stderr ? O_APPEND : O_TRUNC);
        int err_fd = open(redirect.stderr_file.c_str(), flags, 0644);
        if (err_fd >= 0) {
          dup2(err_fd, STDERR_FILENO);
          close(err_fd);
        }
      }

      vector<string> tokens = parseArguments(redirect.clean_command);
      if (tokens.empty()) {
        exit(0);
      }

      string cmd_name = tokens[0];
      bool is_builtin = false;
      for (const string &b : BUILTINS) {
        if (cmd_name == b) {
          is_builtin = true;
          break;
        }
      }

      if (is_builtin) {
        string out = evaluateCommand(redirect.clean_command);
        if (!out.empty()) {
          cout << out << endl;
        }
        exit(0);
      } else {
        vector<char *> args;
        for (size_t t = 0; t < tokens.size(); ++t) {
          args.push_back(const_cast<char *>(tokens[t].c_str()));
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());
        cerr << args[0] << ": command not found" << endl;
        exit(127);
      }
    } else { // Parent process
      pids.push_back(pid);
      if (i > 0) {
        close(prev_fd);
      }
      if (i < num_stages - 1) {
        close(pipefd[1]);
        prev_fd = pipefd[0];
      }
    }
  }

  for (pid_t pid : pids) {
    int status;
    waitpid(pid, &status, 0);
  }
}