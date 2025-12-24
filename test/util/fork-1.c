
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

    // Approach 1 (works)
    // write(pipefd[1], msg, strlen(msg)+1);
    // close(pipefd[1]);
    // Approach 2 (works)
    // dup2(pipefd[1], 0);
    // write(0, msg, strlen(msg)+1);


int
main()
{
  printf("fork-1 ...\n");
  int pipefd[2]; // 0=read, 1=write
  int rc = pipe(pipefd);
  if (rc == -1)
  {
    perror("mcsh: ");
    return EXIT_FAILURE;
  }
  pid_t pid = fork();
  if (pid != 0)
  {
    printf("parent\n");
    close(pipefd[1]);
    char msg[64];
    int count = read(pipefd[0], &msg[0], 64);
    printf("parent: read: %i '%s'\n", count, msg);
    // close(pipefd[1]);
    int wstatus;
    waitpid(pid, &wstatus, 0);
  }
  else
  {
    printf("child\n");
    close(pipefd[0]);
    dup2(pipefd[1], 1);
    close(pipefd[1]);
    char* msg = "meSSage!";

    // WORKS:
    size_t count = strlen(msg)+1;
    ssize_t n = write(1, msg, count); // YES
    assert(n == count);
    // printf("howdy\n"); // YES
    // system("echo hello");  // YES
    // execlp("sh", "sh", "-c", "echo hello", (char*) NULL); // YES
    // execlp("echo", "echo", "-n", "hello", (char*) NULL); // YES

    close(pipefd[1]);
  }

  return 0;
}
