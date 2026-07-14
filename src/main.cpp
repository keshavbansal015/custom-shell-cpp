#include <iostream>
#include <string>


std::string readInput() {
  using namespace std;
  cout << "$ ";
  string command;
  getline(cin, command);
  return command;
}

std::string evaluateCommand(std::string command) {
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
