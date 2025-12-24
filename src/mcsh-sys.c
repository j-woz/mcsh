
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "mcsh.h"
#include "mcsh-sys.h"

#include "buffer.h"

static const int chunk = 128;

bool
mcsh_exec(UNUSED mcsh_module* module,
          char* cmd,
          char** a,
          mcsh_value** output,
          mcsh_status* status)
{
  // Need to initialize this for GCC 11.4.0 (Dunedin Ubuntu 22.04.4)
  int exitcode = 0;
  pid_t pid = fork();
  if (pid != 0)
  {
    show("pid: %i", pid);
    int status;
    waitpid(pid, &status, 0);
    exitcode = WEXITSTATUS(status);
    show("exitcode: %i", exitcode);
  }
  else
  {
    int err = execv(cmd, a);
    if (err != 0)
    {
      perror("mcsh");
      exit(1);
    }
  }

  mcsh_value* result = mcsh_value_new_int(exitcode);
  maybe_assign(output, result);
  status->code = MCSH_OK;
  return true;
}

static bool subcmd_parent(mcsh_module* module, mcsh_value** output,
                          int* pipefd, pid_t pid);

static bool subcmd_child(mcsh_module* module, mcsh_stmts* stmts,
                         mcsh_status* status, int* pipefd);

bool
mcsh_subcmd_capture(mcsh_module* module,
                    mcsh_stmts* stmts,
                    mcsh_value** output,
                    mcsh_status* status)
{
  printf("subcmd_capture ...\n");
  int pipefd[2]; // 0=read, 1=write
  int rc;
  rc = pipe(pipefd);
  assert(rc == 0);
  pid_t pid = fork();
  if (pid != 0)
  {
    rc = subcmd_parent(module, output, pipefd, pid);
    assert(rc);
  }
  else
  {
    rc = subcmd_child(module, stmts, status, pipefd);
    assert(rc);
  }
  status->code = MCSH_OK;
  return true;
}

bool
subcmd_parent(mcsh_module* module, mcsh_value** output, int* pipefd,
              pid_t pid)
{
  buffer B;
  buffer_init(&B, chunk);
  printf("parent\n");
  close(pipefd[1]);
  char t[chunk];
  size_t total = 0;
  while (true)
  {
    memset(t, '\0', chunk);
    ssize_t count = read(pipefd[0], t, 64);
    if (count == 0)  // EOF
      break;
    if (count == -1)
    {
      perror("mcsh:");
      abort();
    }
    buffer_cat(&B, t);
    total += count;
  }
  char* data = buffer_dup(&B);
  // printf("parent: read: %zi '%s'\n", total, data);
  printf("parent: read: %zi\n", total);
  buffer_finalize(&B);
  *output = mcsh_value_new_string(module->vm, data);
  close(pipefd[0]);
  int wstatus;
  waitpid(pid, &wstatus, 0);
  return true;
}

bool
subcmd_child(mcsh_module* module, mcsh_stmts* stmts,
             mcsh_status* status, int* pipefd)
{
  // printf("child\n");
  close(pipefd[0]);
  dup2(pipefd[1], 1);
  close(pipefd[1]);

  // WORKS:
  // write(1, msg, strlen(msg)+1);

  // system("echo hello");  // YES
  // execlp("sh", "sh", "-c", "echo hello", (char*) NULL); // YES
  // execlp("echo", "echo", "-n", "hello", (char*) NULL); // YES

  bool rc = mcsh_stmts_execute(module, stmts, NULL, status);
  CHECK(rc, "child error");

  close(pipefd[1]);
  return true;
}

static bool bg_parent(mcsh_module* module,
                      mcsh_value** output,
                      pid_t pid);

static void bg_child(mcsh_module* module, mcsh_stmts* stmts,
                     mcsh_status* status);


bool
mcsh_bg(mcsh_module* module,
        mcsh_stmts* stmts,
        mcsh_value** output,
        mcsh_status* status)
{
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_INFO,
           "bg ...");
  bool rc;
  pid_t pid = fork();
  if (pid != 0)
  {
    rc = bg_parent(module, output, pid);
    assert(rc);
  }
  else
  {
    bg_child(module, stmts, status);
    // bg_child should not return!
    assert(false);
  }
  status->code = MCSH_OK;
  return true;
}

static bool
bg_parent(mcsh_module* module, mcsh_value** output, pid_t pid)
{
  mcsh_value* result = mcsh_value_new_int(pid);
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_INFO,
           "bg_parent(): child is: %i\n", pid);
  fflush(stdout);
  // printf("bg: parent: output: %p\n", output);
    fflush(stdout);
    // printf("bg: parent: *output: %p\n", *output);
    fflush(stdout);
  maybe_assign(output, result);
  return true;
}

static void
bg_child(mcsh_module* module, mcsh_stmts* stmts,
         mcsh_status* status)
{
  pid_t pid = getpid();
  mcsh.pid                    = pid;
  module->vm->logger.pid      = pid;
  module->vm->logger.show_pid = true;
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_INFO,
           "bg_child: pid: %i", pid);
  bool rc = mcsh_stmts_execute(module, stmts, NULL, status);
  valgrind_assert_msg(rc, "bg child error!");
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_INFO,
           "bg_child: exit.");
  exit(EXIT_SUCCESS);
}
