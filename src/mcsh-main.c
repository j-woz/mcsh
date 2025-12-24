
/**
   MCSH MAIN
*/

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// #include "strlcpyj.h"

#include "buffer.h"
#include "log.h"
#include "util.h"

#include "mcsh.h"
#include "mcsh-iface.h"

static bool mcsh_start(mcsh_cmd_line* cmd, mcsh_vm* vm,
                       mcsh_value** value, mcsh_status* status);

static bool report_value(bool result, mcsh_value* value);

/* static void do_result(bool result, mcsh_status* status, */
/*                       int* exit_status); */

int
main(int argc, char* argv[])
{
  int exit_status = EXIT_SUCCESS;
  setbuf(stdout, NULL);

  mcsh_cmd_line cmd;
  mcsh_cmd_line_init(&cmd);

  bool rc;
  rc = mcsh_init();
  if (!rc) fail("mcsh: could not initialize!");
  mcsh_log(&mcsh.logger, MCSH_LOG_SYSTEM, MCSH_WARN,
           "MCSH START");

  rc = mcsh_parse_options(argc, argv, &cmd);

  if (cmd.mode == MCSH_MODE_PROTO)
    mcsh_parse_args(argc, argv, &cmd);

  mcsh_vm vm;
  mcsh_vm_init_cmd(&vm, &cmd);

  mcsh_value* value = NULL;
  mcsh_status status;
  mcsh_status_init(&status);

  // Execute!
  bool result;
  result = mcsh_start(&cmd, &vm, &value, &status);
  result = report_value(result, value);
  mcsh_final_status(result, &status, &exit_status);

  mcsh_vm_stop(&vm);
  mcsh_cmd_line_finalize(&cmd);
  free(cmd.argv);

  mcsh_log(&mcsh.logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "EXIT: code=%i", exit_status);

  mcsh_finalize();
  return exit_status;
}

bool mcsh_start_interactive(mcsh_module* module,
                            mcsh_value** value, mcsh_status* status);
bool mcsh_start_slurp(mcsh_cmd_line* cmd, mcsh_module* module,
                      mcsh_value** value, mcsh_status* status);
bool mcsh_start_string(mcsh_cmd_line* cmd, mcsh_module* module,
                       mcsh_value** value, mcsh_status* status);

bool
mcsh_start(mcsh_cmd_line* cmd, mcsh_vm* vm,
           mcsh_value** value, mcsh_status* status)
{
  bool result;
  mcsh_module* module = vm->main;
  switch (cmd->mode)
  {
    case MCSH_MODE_INTERACTIVE:
      result = mcsh_start_interactive(module, value, status);
      break;
    case MCSH_MODE_SCRIPT:
      result = mcsh_start_slurp(cmd, module, value, status);
      break;
    case MCSH_MODE_STRING:
      result = mcsh_start_string(cmd, module, value, status);
      break;
    default:
      valgrind_fail_msg("Unknown mode!");
  }
  CHECK(result, "mcsh_start: failed!");
  return result;
}

bool
mcsh_start_interactive(mcsh_module* module,
                       mcsh_value** value,
                       mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;
  mcsh_log(logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "interactive session start...");
  char* code;
  bool b;
  bool result = true;
  while (true)
  {
    b = mcsh_iface_get("mcsh> ", &code);
    assert(b);
    if (code == NULL)
      break;

    mcsh_module_parse("(stdin)", "mcsh.stdin", code, module);
    mcsh_log_line(module, code);

    status->code = MCSH_OK;
    result = mcsh_module_execute(module, value, status);
    free(code);

    if (status->code != MCSH_OK) break;
  }
  mcsh_log(logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "interactive session stop.");
  return result;
}

bool
mcsh_start_slurp(mcsh_cmd_line* cmd, mcsh_module* module,
                 mcsh_value** value, mcsh_status* status)
{
  buffer code;
  buffer_init(&code, 1024);
  bool b = slurp_buffer(cmd->stream, &code);
  assert(b);

  if (cmd->stream != stdin) fclose(cmd->stream);

  mcsh_log(&module->vm->logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "parse: %s", cmd->argv[0]);

  mcsh_module_parse(cmd->argv[0], "mcsh.main", code.data, module);

  if (mcsh_log_check(&module->vm->logger, MCSH_LOG_PARSE, MCSH_FATAL))
    mcsh_module_print(module, 0);

  status->code = MCSH_OK;
  bool result = mcsh_module_execute(module, value, status);
  buffer_finalize(&code);
  return result;
}

bool
mcsh_start_string(mcsh_cmd_line* cmd, mcsh_module* module,
                  mcsh_value** value, mcsh_status* status)
{
  buffer code;
  buffer_init(&code, 1024);

  bool result = false;
  for (size_t i = 0; i < cmd->argc; i++)
  {
    mcsh_module_parse("(command_line)", "mcsh.command_line",
                      cmd->argv[i], module);
    status->code = MCSH_OK;
    result = mcsh_module_execute(module, value, status);
  }

  return result;
}

/** Print result of MCSH execution
    TODO: Does not happen by default, provide option.  See TODO.txt
*/
static bool
report_value(bool result, mcsh_value* value)
{
  if (! result) return false;
  if (value != NULL)
  {
    // list_array* L;
    switch (value->type)
    {
      case MCSH_VALUE_STRING:
        // printf("mcsh: result: \"%s\"\n", value->string);
        break;
      case MCSH_VALUE_LIST:
        ;
        // L = value->list;
        // printf("mcsh: result: list size: %zi\n", L->size);
        break;
      default:
        ; // printf("mcsh: result: OTHER\n");
    }
  }
  return true;
}
