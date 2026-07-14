#include <iostream>
#include <string>


std::string readInput() {
  std::cout << "$"<<std::endl;
  std::string command;
  std::getline(std::cin, command);
  return command;
}

std::string evaluateCommand(std::string command) {
  return command + ": command not found";
}

void printOutput(std::string output) {
  std::cout<<output<<std::endl;
}

void shellLoop() {
  while(true) {
    std::string command = readInput();
    std::string output = evaluateCommand(command);
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
