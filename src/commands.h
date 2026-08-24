#ifndef COMMANDS_H
#define COMMANDS_H
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
inline const vector<string> BUILTINS = {"echo",     "type", "pwd", "cd",
                                        "complete", "jobs", "history", "exit"};
struct Job {
  int job_number;
  pid_t pid;
  string command;
  string status;
};
extern unordered_map<string, string> completion_specs;
extern vector<Job> background_jobs;
string findInPATH(const string &command);
string typeCommand(const string &command);
string execute(const string &command);
string pwd();
string cd(const string &directory);
string echo(const string &message);
string evaluateCommand(const string &command);
void reapJobs();
void printAndClearCompletedJobs();
string completeCommand(const string &command);
void printOutput(const string &output);

#endif
