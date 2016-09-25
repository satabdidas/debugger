#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>


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

int count_instructions(pid_t pid) {
  int wait_status;
  unsigned counter = 0;

  wait(&wait_status);
  while(WIFSTOPPED(wait_status)) {
    ++counter;
    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0) {
      perror("Error in ptrace\n");
      return;
    }

    wait(&wait_status);
  }
  /* printf("The child executed with %d instructions\n", pid); */
  return counter;
}

unsigned set_breakpoint(pid_t pid, unsigned addr) {
  unsigned data = ptrace(PTRACE_PEEKTEXT, pid, (void *) addr, 0);
  unsigned old_data = data;

  printf("Setting a breakpoint at %x\n", addr);
  printf("Value at the address %x %x\n", addr, data);
  data = (data & ~0xff) | 0xcc; 
  ptrace(PTRACE_POKETEXT, pid, (void *)addr, data);
  printf("Value at the address %x after setting the int 3 opcode %x\n", addr, data);
  printf("Done setting the breatpoint at %x\n\n", addr);
  return old_data;
}

void set_rip(pid_t pid, unsigned addr) {
  struct user_regs_struct regs;

  printf("Setting the instruction pointer to address %x\n", addr);
  memset(&regs, 0, sizeof(regs));
  ptrace(PTRACE_GETREGS, pid, NULL, &regs);
  regs.eip = addr;
  ptrace(PTRACE_SETREGS, pid, NULL, &regs);
  printf("Done setting the instruction pointer\n\n");
}

unsigned get_rip(pid_t pid) {
  struct user_regs_struct regs;
  ptrace(PTRACE_GETREGS, pid, NULL, &regs);
  return regs.eip;
}

void run_debugger(pid_t pid) {
  int wait_status;
  unsigned addr = 0x80483e9;

  /* Wait for the child to stop first */
  wait(&wait_status);

  /* Set breakpoint at the address - 80483e9 */
  unsigned old_data = set_breakpoint(pid, addr);
  ptrace(PTRACE_CONT, pid, NULL, NULL);
  wait(&wait_status);

  printf("Hit a breakpoint\n");

  /* Restore the code at the address - 80483e9 */
  ptrace(PTRACE_POKETEXT, pid, (void *)addr, old_data);
  printf("Restored the old value %x at the address %x\n", old_data, addr);

  /* Set the correct address at the eip/rip to resume the execution */
  /* after the breakpoint has been hit. */
  set_rip(pid, addr);
  ptrace(PTRACE_CONT, pid, NULL, NULL);

  wait(&wait_status);

  if (WIFEXITED(wait_status)) {
    printf("Child process exited\n");
  } else {
    printf("Unexpected signal");
  }
}
