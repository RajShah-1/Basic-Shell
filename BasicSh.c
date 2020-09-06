#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define PATH_MAX 100            // Max length for the cwd
#define MACHINE_LENGTH_MAX 100  // Max length for the machine length
#define PROMPT_LENGTH 204       // Max length of the prompt
#define CMD_MAX 100             // Max length for the cmd_buff
#define ARG_BUFSIZE 64          // Max number of supported arg
#define ARG_DELIM " \t\r\n\a"   // Delimeters to seperate args
#define MAX_BG_PROCS 100        // Max num of background jobs at any given time
#define MAX_HIST 10  // Max num of history entries stored in the queue

typedef struct procInfo {
  char cmd_buff[CMD_MAX];
  unsigned int id;
  pid_t pid;
  struct procInfo* next;
} procInfo;

typedef struct {
  procInfo* front;
  procInfo* back;
  int size;
  int maxSize;
  int processID;
} Queue;

// Global Variables
Queue history;
Queue jobs;

void cd(char* argPath, char* cwd);
char** parseCMD(char* cmd_buff);
char* buildPrompt(char* prompt, char* hostname, char* path);
int isBuiltIn(char** args);
void runBuiltIn(char** args, char* path);
int isBackgroundJob(char** args);
void sigchldHandler(int sigNum);
void execUserProcess(char** args);

// Queue operations
procInfo* newProcNode(const char* cmd_buff, pid_t pid, int id);
void enqueue(Queue* Q, const char* cmd_buff, pid_t pid);
void removePID(Queue* Q, pid_t pid);
char* getPrevCMD(Queue* Q, int id);
void printJobs(Queue* Q);
void printHistory(Queue* Q);

int main() {
  char* cmd_buff;
  char cmd_buff_tmp[CMD_MAX];
  char cmd_buff_args[CMD_MAX];
  char path[PATH_MAX];
  char hostname[MACHINE_LENGTH_MAX];
  char prompt[PROMPT_LENGTH];
  pid_t childPid;
  int historyID;

  history.front = history.back = NULL;
  history.processID = history.size = 0;
  history.maxSize = MAX_HIST;

  jobs.front = jobs.back = NULL;
  jobs.processID = jobs.size = 0;
  jobs.maxSize = MAX_BG_PROCS;

  // Get the host name
  gethostname(hostname, MACHINE_LENGTH_MAX);

  // Set up SIGCHLD handler
  signal(SIGCHLD, sigchldHandler);

  while (1) {
    // Build path
    if (getcwd(path, PATH_MAX) == NULL) {
      printf("Error\n");
      continue;
    }

    cmd_buff = readline(buildPrompt(prompt, hostname, path));
    strcpy(cmd_buff_tmp, cmd_buff);
    strcpy(cmd_buff_args, cmd_buff);

    // Support up and down arrows to show previously used commands
    if (strlen(cmd_buff) > 0) {
      // Add to history only if the command is not empty
      add_history(cmd_buff);
    }

    // free cmd_buff (allocated by readLine library)
    free(cmd_buff);

    // Parse cmd_buff
    char** args = parseCMD(cmd_buff_args);
    // Sanity check
    if (args[0] == NULL) {
      printf("Enter a valid command!\n");
      continue;
    }

    if (args[0][0] == '!') {
      char* tmp = args[0] + 1;
      historyID = atoi(tmp);
      // printf("History-ID: %d\n", historyID);
      cmd_buff = getPrevCMD(&history, historyID);
      free(args);
      if (cmd_buff == NULL) {
        printf("Command does not exist in the history!\n");
        continue;
      }
      strcpy(cmd_buff_tmp, cmd_buff);
      enqueue(&history, cmd_buff_tmp, -1);

      printf("Executing: %s\n", cmd_buff_tmp);
      args = parseCMD(cmd_buff_tmp);
      if (args[0] == NULL) {
        continue;
      }
    } else {
      enqueue(&history, cmd_buff_tmp, -1);
    }

    if (isBuiltIn(args)) {
      if (!strcmp(args[0], "cd")) {
        cd(args[1], path);
      } else if (!strcmp(args[0], "ls")) {
        system("ls");
      } else if (!strcmp(args[0], "exit")) {
        if (jobs.size != 0) {
          printf("There are %d process(es) running in the background. ",
                 jobs.size);
          printf("Run jobs command to view them. ");
          printf(
              "You must kill them/ wait for their completion before exting the "
              "shell.\n");
        } else {
          printf("Exiting...\n");
          free(args);
          break;
        }
      } else if (!strcmp(args[0], "jobs")) {
        printJobs(&jobs);
      } else if (!strcmp(args[0], "history")) {
        printHistory(&history);
      } else if (!strcmp(args[0], "kill")) {
        // Kill the child!
        if (args[1] == NULL || args[1][0] != '%') {
          printf("Invalid args\n");
        } else if (kill(atoi(args[1] + 1), SIGKILL) == -1) {
          printf("Invalid pid/permission denied\n");
        }
      }
      continue;
    }

    // Create a fork and execute the user command
    childPid = fork();
    if (childPid == -1) {
      // ERR
      printf("ERROR...");
      exit(EXIT_FAILURE);
    } else if (childPid == 0) {
      // child
      execUserProcess(args);
      return 0;
    } else {
      // Check whether the child process is a background job
      if (isBackgroundJob(args)) {
        // Record as a background process
        printf("Background Process started\n");
        enqueue(&jobs, cmd_buff, childPid);
      } else {
        waitpid(childPid, NULL, 0);
      }
    }
    free(args);
  }
  return 0;
}

void execUserProcess(char** args) {
  // Open infile and outfile
  FILE *fd1 = NULL, *fd0 = NULL;
  int i = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], ">") == 0) {
      fd1 = fopen(args[i + 1], "w");
      // printf("Outfile: %s\n", args[i + 1]);
      if (fd1 == NULL) {
        printf("Can't open the file!\n");
        printf("Terminating the process...\n");
        exit(EXIT_FAILURE);
      }
      dup2(fileno(fd1), STDOUT_FILENO);
      fclose(fd1);  
    }

    if (strcmp(args[i], "<") == 0) {
      fd0 = fopen(args[i + 1], "r");
      // printf("Infile: %s\n", args[i + 1]);
      if (fd0 == NULL) {
        printf("Can't open the file\n");
        printf("Terminating the process...\n");
        exit(0);
      }
      dup2(fileno(fd0), STDIN_FILENO);
      fclose(fd0);
    }
    i++;
  }

  execvp(args[0], args);
}

// Queue operations
void enqueue(Queue* Q, const char* cmd_buff, pid_t pid) {
  procInfo* newProc = newProcNode(cmd_buff, pid, Q->processID++);

  if (Q->front == NULL) {
    Q->size++;
    Q->back = Q->front = newProc;
    return;
  }

  if (Q->size >= Q->maxSize) {
    procInfo* tmp = Q->front;
    Q->front = Q->front->next;
    free(tmp);
    Q->size--;
  }

  Q->size++;
  Q->back->next = newProc;
  Q->back = Q->back->next;
}

void removePID(Queue* Q, pid_t pid) {
  // Find and delete the node with pid == pid
  if (Q->front == NULL) {
    printf("Warning: Empty queue!");
    return;
  }

  procInfo *curr = Q->front, *prev;
  if (curr->pid == pid) {
    procInfo* tmp = curr;
    Q->front = Q->front->next;
    if (Q->front == NULL) {
      Q->back = NULL;
    }
    free(tmp);
    Q->size--;
    return;
  }

  while (curr && curr->pid != pid) {
    prev = curr;
    curr = curr->next;
  }
  if (curr == NULL) {
    printf("Warning: pid not found!");
    return;
  }
  Q->size--;
  prev->next = curr->next;
  free(curr);
  if (prev->next == NULL) {
    Q->back = prev;
  }
}

char* getPrevCMD(Queue* Q, int id) {
  if (Q->front == NULL) return NULL;
  if (id == -1) return Q->back->cmd_buff;

  procInfo* tmp = Q->front;
  while (tmp) {
    if (tmp->id == id) return tmp->cmd_buff;
    tmp = tmp->next;
  }
  return NULL;
}

void printJobs(Queue* Q) {
  procInfo* tmp = Q->front;
  printf("Background jobs: \n");
  while (tmp) {
    printf("NUM(pid): %d - %s\n", tmp->pid, tmp->cmd_buff);
    tmp = tmp->next;
  }
}

void printHistory(Queue* Q) {
  procInfo* tmp = Q->front;
  printf("%d Previous Commands: \n", Q->size);
  while (tmp) {
    printf("ID: %d - %s\n", tmp->id, tmp->cmd_buff);
    tmp = tmp->next;
  }
}

int isBackgroundJob(char** args) {
  int index = 0;
  char* prevArg = NULL;
  while (args[index + 1] != NULL) {
    index++;
  }
  return strcmp(args[index], "&") ? 0 : 1;
}

procInfo* newProcNode(const char* cmd_buff, pid_t pid, int id) {
  procInfo* newProc = (procInfo*)malloc(sizeof(procInfo));
  strcpy(newProc->cmd_buff, cmd_buff);
  newProc->id = id;
  newProc->pid = pid;
  newProc->next = NULL;

  return newProc;
}

// Checks whether args[0] is a builtin command or not
int isBuiltIn(char** args) {
  if (args[0] == NULL) return 0;
  if (!strcmp(args[0], "cd") || !strcmp(args[0], "ls") ||
      !strcmp(args[0], "exit") || !strcmp(args[0], "jobs") ||
      !strcmp(args[0], "history") || !strcmp(args[0], "kill") ||
      !strcmp(args[0], "!"))
    return 1;

  return 0;
}

// Returns pointer to prompt just for convenience
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
// (Does not support white spaces in arguments as of now)
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
    printf("Enter a valid path!\n");
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
    printf("Enter a valid path!\n");
  }
}

// Waits for all the terminated childs to avoid "Zombies"
void sigchldHandler(int signum) {
  while (1) {
    pid_t pid = waitpid(-1, NULL, WNOHANG);
    if (pid <= 0) {
      break;
    }
    removePID(&jobs, pid);
  }
}