#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATH_MAX 100            // Max length for the cwd
#define MACHINE_LENGTH_MAX 100  // Max length for the machine length
#define PROMPT_LENGTH 204       // Max length of the prompt
#define ARG_BUFSIZE 64          // Max number of supported arg
#define ARG_DELIM " \t\r\n\a"

void cd(char* argPath, char* cwd);
char** parseCMD(char* cmd_buff);
char* buildPrompt(char* prompt, char* hostname, char* path);
int isBuiltIn(char** args);
void runBuiltIn(char** args, char* path);

int main() {
  char* cmd_buff;
  char path[PATH_MAX];
  char hostname[MACHINE_LENGTH_MAX];
  char prompt[PROMPT_LENGTH];
  int childPid;
  int isBackground = 0;

  // Get the host name
  gethostname(hostname, MACHINE_LENGTH_MAX);

  while (1) {
    // Build path
    if (getcwd(path, PATH_MAX) == NULL) {
      printf("Error\n");
      continue;
    }

    cmd_buff = readline(buildPrompt(prompt, hostname, path));

    // Support up and down arrows to show previously used commands
    if (strlen(cmd_buff) > 0) {
      // Add to history only if the command is not empty
      add_history(cmd_buff);
      // Custom function to keep track of the history
    }

    // Parse cmd_buff
    char** args = parseCMD(cmd_buff);
    // Sanity check
    if (args[0] == NULL) {
      printf("Enter a valid command!\n");
      continue;
    }

    if (isBuiltIn(args)) {
      if (!strcmp(args[0], "cd")) {
        cd(args[1], path);
      } else if (!strcmp(args[0], "ls")) {
        system("ls");
      } else if (!strcmp(args[0], "exit")) {
        printf("Exiting...\n");
        free(args);
        // free(cmd_buff);
        break;
      } else if (!strcmp(args[0], "jobs")) {
      } else if (!strcmp(args[0], "history")) {
      } else if (!strcmp(args[0], "kill")) {
      } else if (!strcmp(args[0], "!")) {
      }
      continue;
    }
    // Check whether the job is a background job or not
    int index = 0;
    char* prevArg = NULL;
    while(args[index+1] != NULL){
      index++;
    }
    isBackground = strcmp(args[index], "&") ? 0:1;

    // Create a fork and execute the user command
    childPid = fork();
    if (childPid == -1) {
      // ERR
      printf("ERROR...");
      exit(EXIT_FAILURE);
    } else if (childPid == 0) {
      // child
    } else{
      // Check whether the child process is a background job

    }

    // free(cmd_buff);
    free(args);
  }
  return 0;
}

int isBuiltIn(char** args) {
  if (args[0] == NULL) return 0;
  if (!strcmp(args[0], "cd") || !strcmp(args[0], "ls") ||
      !strcmp(args[0], "exit") || !strcmp(args[0], "jobs") ||
      !strcmp(args[0], "history") || !strcmp(args[0], "kill") ||
      !strcmp(args[0], "!"))
    return 1;

  return 0;
}

// Returns pointer to prompt
char* buildPrompt(char* prompt, char* hostname, char* path) {
  // Build the prompt (with colors!)
  strcpy(prompt, "\033[1;32m");  // Bold green
  strcat(prompt, hostname);
  strcat(prompt, ": ");
  strcat(prompt, "\033[1;34m");  // Bold Blue
  strcat(prompt, path);
  strcat(prompt, ">>");
  strcat(prompt, "\033[0m");  // Reset
  return prompt;
}

// Parses the entered command into args
// (Does not support white spaces in argument as of now)
char** parseCMD(char* cmd_buff) {
  char** args = (char**)malloc(ARG_BUFSIZE * sizeof(char));
  int position = 0;
  // The arguments are seperated by whitespace

  char* token = strtok(cmd_buff, ARG_DELIM);

  // Iteatively add all arguments to the args array
  while (token != NULL) {
    args[position++] = token;
    token = strtok(NULL, ARG_DELIM);
  }

  // Null terminate args
  args[position] = NULL;

  return args;
}

// changes current directory
// Supports both relative and absolute paths
void cd(char* argPath, char* cwd) {
  if (argPath == NULL) {
    printf("Enter a valid path!");
    return;
  }

  char path[PATH_MAX];
  strcpy(path, argPath);

  int status = 0;
  if (argPath[0] != '/') {
    // Handle relative paths
    strcat(cwd, "/");
    strcat(cwd, path);
    status = chdir(cwd);
  } else {
    // Handle absolute paths
    status = chdir(argPath);
  }
  if (status == -1) {
    printf("Enter a valid path!");
  }
}