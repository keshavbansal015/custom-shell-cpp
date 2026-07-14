#include <iostream>
#include <string>


#define SHELL_COMMANDS {"echo" , "type" , "exit"}
#define SHELL_PROMPT "$ "

std::string readInput() {
  using namespace std;
  cout << SHELL_PROMPT;
  string command;
  getline(cin, command);
  return command;
}

std::string evaluateCommand(std::string command) {
  using namespace std;
  if (command == "exit") {
    exit(0); 
  } else if (command.substr(0, 4) == "echo" && command[4] == ' ') {
    return command.substr(5);
  } else if (command.substr(0, 4) == "type" && command[4] == ' '){
    
  }
  return command + ": command not found";
}

void printOutput(std::string output) {
  using namespace std;
  cout<<output<<endl;
}


std::string typeCommand (std::string command) {
  using namespace std;
  
  for (string shellCommand: SHELL_COMMANDS) {
    if (command.substr(0, shellCommand.length()) == shellCommand) {
      return shellCommand + " is a shell builtin";
    }
  }
  return command + ": command not found";
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
