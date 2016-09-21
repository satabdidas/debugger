#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>


void run_child(const char* childProg);
void run_debugger(pid_t pid);

int main(int argc, char* argv[]) {
  pid_t pid;

  if (argc < 2) {
    printf("Give at least one argument which is the program name\n");
    return -1;
  }

  pid = fork();
  if (pid == 0) {
    // child process
    run_child(argv[1]);
  } else if (pid >= 1) {
    // parent process
    run_debugger(pid);
  } else {
    // error in forking
    perror("Error in forking the child");
    return -1;
  }
  return 0;
}

void run_child(const char* childProg) {
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    perror("Error in ptrace\n");
    return;
  }
  execl(childProg, childProg, NULL);
}

void run_debugger(pid_t pid) {
  int wait_status;
  unsigned counter = 0;

  printf("Debug: Start count = %d\n", counter);

  wait(&wait_status);
  while(WIFSTOPPED(wait_status)) {
    ++counter;
    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0) {
      perror("Error in ptrace\n");
      return;
    }

    wait(&wait_status);
  }
  printf("The child executed with %d instructions\n", pid);
}
