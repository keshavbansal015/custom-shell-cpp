#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <ranges>
#include <filesystem>


#define SHELL_COMMANDS {"echo" , "type", "pwd" , "exit"}
#define SHELL_PROMPT "$ "

std::string readInput() {
  using namespace std;
  cout << SHELL_PROMPT;
  string command;
  getline(cin, command);
  return command;
} 

std::string findInPATH(std::string command) {
  using namespace std;  
  char* path = getenv("PATH");
  string pathStr = path;
  
  size_t prevPos = 0;
  size_t currPos = pathStr.find(":");
  
  // Traverse through all the directories in the PATH
  while(currPos != string::npos){
    string currDir = pathStr.substr(prevPos, currPos-prevPos);
    string fullPath = currDir + "/" + command;
    
    if(access(fullPath.c_str(), F_OK) == 0 && access(fullPath.c_str(), X_OK) == 0) {
      return fullPath;
    }

    prevPos = currPos+1;
    currPos = pathStr.find(":", prevPos);
  }
  return "";
}


std::string typeCommand(std::string command) {
  using namespace std;
  
  // check if the command is a shell builtin
  for (string shellCommand: SHELL_COMMANDS) {
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

std::string execute(std::string command) {
  using namespace std;
  
  // using execvp
  vector<string> tokens = command 
                | views::split(' ') 
                | ranges::to<vector<string>>();

  // converting vector of string to array of char*
  char** args = new char*[tokens.size() + 1];
  for (size_t i = 0; i < tokens.size(); i++) {
    args[i] = (char*)tokens[i].c_str();
  }
  args[tokens.size()] = nullptr;
  
  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args);
    // If execvp returns, it failed
    cerr << args[0] << ": command not found" << endl;
    exit(127);
  } else if (pid < 0) {
    delete[] args;
    return "Error in forking";
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
  delete[] args;
  return "";
}

std::string pwd() {
   using namespace std;
   filesystem::path cwd = std::filesystem::current_path();
   return cwd.string();
}

std::string evaluateCommand(std::string command) {
  using namespace std;
  if (command == "exit") {
    exit(0); 
  } else if (command.substr(0, 4) == "echo" && command[4] == ' ') {
    return command.substr(5);
  } else if (command.substr(0, 4) == "type" && command[4] == ' '){
    return typeCommand(command.substr(5));
  } else if (command == "pwd") {
    return pwd();
  } else {
    return execute(command);
  }
}

void printOutput(std::string output) {
  using namespace std;
  if (!output.empty()) {
    cout << output << endl;
  }
}


void shellLoop() {
  using namespace std;
  while(true) {
    string command = readInput();
    string output = evaluateCommand(command);
    printOutput(output);
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  shellLoop();
}
