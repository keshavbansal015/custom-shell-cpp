#include <iostream>
#include <string>
#include <unistd.h>



#define SHELL_COMMANDS {"echo" , "type" , "exit"}
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
      return command + " is " + fullPath;
    }

    prevPos = currPos+1;
    currPos = pathStr.find(":", prevPos);
  }
  
  return command + ": not found";
}


std::string typeCommand(std::string command) {
  using namespace std;
  
  // check if the command is a shell builtin
  for (string shellCommand: SHELL_COMMANDS) {
    if (command.substr(0, shellCommand.length()) == shellCommand) {
      return shellCommand + " is a shell builtin";
    }
  }

  return findInPATH(command);
}

std::string evaluateCommand(std::string command) {
  using namespace std;
  if (command == "exit") {
    exit(0); 
  } else if (command.substr(0, 4) == "echo" && command[4] == ' ') {
    return command.substr(5);
  } else if (command.substr(0, 4) == "type" && command[4] == ' '){
    return typeCommand(command.substr(5));
  }
  return command + ": command not found";
}

void printOutput(std::string output) {
  using namespace std;
  cout<<output<<endl;
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
