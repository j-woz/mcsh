
/**
   MCSH CALC
   Makes the program mcc
*/

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "mcsh.h"
#include "mcsh-iface.h"
#include "mcsh-preprocess.h"

static bool mcc_start(mcsh_vm* vm, mcsh_cmd_line* cmd,
                      mcsh_value** value, mcsh_status* status);

typedef struct
{
  mcsh_mode mode;
  char script[PATH_MAX];
} mcc_args;

int
main(int argc, char* argv[])
{
  int exit_status = EXIT_SUCCESS;

  setbuf(stdout, NULL);
  printf("MCSH CALC\n");

  assert(argc > 0);
  assert(argv[0] != NULL);

  mcsh_init();
  mcsh_vm vm;
  mcsh_vm_init_argv(&vm, argc, argv);

  mcsh_cmd_line cmd;
  mcsh_cmd_line_init(&cmd);

  bool rc = mcsh_parse_options(argc, argv, &cmd);
  if (!rc)
  {
    printf("mcc: option error.\n");
    goto finalize;
  }
  mcsh_parse_args(argc, argv, &cmd);

  mcsh_value* value = NULL;
  mcsh_status status;
  status.code = MCSH_PROTO;

  bool result = mcc_start(&vm, &cmd, &value, &status);

  mcsh_final_status(result, &status, &exit_status);

  mcsh_cmd_line_finalize(&cmd);

  finalize:
  mcsh_finalize();
  return exit_status;
}

static bool mcc_start_interactive(mcsh_module* module,
                                  mcsh_value** value,
                                  mcsh_status* status);

static bool mcc_start_slurp(mcsh_cmd_line* cmd,
                            mcsh_module* module,
                            mcsh_value** value, mcsh_status* status);

static bool
mcc_start(mcsh_vm* vm, mcsh_cmd_line* cmd,
          mcsh_value** value, mcsh_status* status)
{
  bool result;
  mcsh_module* module = vm->main;
  switch (cmd->mode)
  {
    case MCSH_MODE_INTERACTIVE:
      result = mcc_start_interactive(module, value, status);
      break;
    case MCSH_MODE_SCRIPT:
      result = mcc_start_slurp(cmd, module, value, status);
      break;
    default:
      valgrind_fail_msg("Unknown mode!");
  }
  CHECK(result, "mcsh_start: failed!");
  return result;
}

#define VALUE_STRING_MAX 1024

static bool mcc_start_interactive(mcsh_module* module,
                                  mcsh_value** value,
                                  mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;
  mcsh_log(logger, MCSH_LOG_SYSTEM, MCSH_WARN,
           "mcc: interactive session start...");
  char* code;
  char value_string[VALUE_STRING_MAX];
  bool b, loop = true;
  bool result = true;
  while (loop)
  {
    b = mcsh_iface_get("mcc> ", &code);
    assert(b);
    if (code == NULL)
      break;

    mcsh_log_line(module, code);

    status->code = MCSH_OK;

    mcsh_node* node;
    mcsh_expr_scan(code, &node, status);
    switch (status->code)
    {
      case MCSH_OK: break;  // Normal case
      case MCSH_EXCEPTION:
        printf("mcc: uncaught exception: line==%i %s\n",
               status->exception->line,
               status->exception->text);
        // Keep looping only if syntax error
        if (strcmp(status->exception->tag, "mcsh.syntax_error") != 0)
          loop = false;
        mcsh_exception_reset(status);
        goto done;
      default:
        // Other errors:
        goto done;
        break;
    }
    // printf("scan ok.\n");

    mcsh_expr* expr;
    mcsh_node_to_expr(node, &expr);
    // printf("translate OK\n");

    // mcsh_expr_print(expr, 0);

    bool rc = mcsh_expr_eval(module->vm, expr, value);
    // printf("execute\n");
    CHECK(rc, "mcsh: expr execution failed!\n");

    mcsh_to_string(logger, value_string, VALUE_STRING_MAX, *value);
    printf("\t ==> %s\n", value_string);

    done:
    free(code);
    if (status->code != MCSH_OK) break;
  }
  mcsh_log(logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "interactive session stop.");
  return result;
}

static bool mcc_start_slurp(mcsh_cmd_line* cmd,
                            mcsh_module* module,
                            mcsh_value** value,
                            mcsh_status* status)
{
  buffer code;
  buffer_init(&code, 1024);
  bool b = slurp_buffer(cmd->stream, &code);
  assert(b);

  printf("read ok\n");

  mcsh_script_preprocess(code.data);
  // strcpy(code.data, "888\n");
  printf("pp: %s\n--\n", code.data);

  mcsh_node* node;
  mcsh_expr_scan(code.data, &node, status);
  printf("scan ok.\n");

  mcsh_expr* expr;
  mcsh_node_to_expr(node, &expr);
  printf("translate OK\n");

  mcsh_expr_print(expr, 0);

  printf("eval: expr: %p\n", expr);
  bool result = mcsh_expr_eval(module->vm, expr, value);
  printf("eval'd\n");
  if (! result)
  {
    printf("mcsh: execution failed!\n");
    return EXIT_FAILURE;
  }

  char value_string[128];
  mcsh_to_string(&module->vm->logger, value_string, 128, *value);
  printf("RESULT: %s\n", value_string);

  buffer_finalize(&code);

  mcsh_expr_finalize(expr);
  return true;
}
