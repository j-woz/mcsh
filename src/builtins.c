
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <libgen.h>

#include "builtins.h"

#include "exceptions.h"
#include "list_i.h"
#include "lookup.h"
#include "table.h"
#include "strlcpyj.h"
#include "util-string.h"
#include "util.h"

#include "mcsh-iface.h"
#include "mcsh-sys.h"

static void builtins_add(void);

/** Builtin-Bundle: The arguments to all builtins */
typedef struct
{
  mcsh_module* module;
  // List of mcsh_value* .  Index 0 is the called builtin name.
  list_array* args;
  mcsh_value** output;
  mcsh_status* status;
} mcsh_bb;

void
mcsh_builtins_init()
{
  mcsh.builtins = table_create(128);
  builtins_add();
}

bool
mcsh_builtins_has(const char* symbol)
{
  return table_contains(mcsh.builtins, symbol);
}

static inline void
bb_init(mcsh_bb* bb, mcsh_module* module, list_array* args,
        mcsh_value** output, mcsh_status* status)
{
  bb->module = module;
  bb->args   = args;
  bb->output = output;
  bb->status = status;
}

bool
mcsh_builtins_execute(mcsh_module* module, list_array* args,
                      mcsh_value** output, mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;

  mcsh_value* command_value = args->data[0];
  char* command = command_value->string;

  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_execute: '%s' ...", command);

  void* p;
  bool b = table_search(mcsh.builtins, command, &p);
  valgrind_assert(b);
  valgrind_assert(p);
  bool (*builtin)(mcsh_bb*) = p;
  mcsh_bb bb;
  bb_init(&bb, module, args, output, status);

  bool rc = builtin(&bb);

  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_execute: '%s' done.", command);
  return rc;
}

/** Maximal length of builtin name including subcommands */
static size_t MAX_COMMAND = 128;

/** Ensure command has argument-count count
    Note that the count is w/o the command itself
 */
#define EXCEPTION_ARGC_EQ(required)                    \
  do {                                                 \
    int given = bb->args->size - 1;                    \
    if (required != given)                             \
    {                                                  \
      mcsh_value* _v = bb->args->data[0];              \
      char* _s = _v->string;                           \
      mcsh_exception_invalid_argc(bb->status, _s,      \
                                  required, given);    \
      return true;                                     \
    } } while (0);

/** Ensure subcommand has argument-count count */
#define EXCEPTION_SUBARGC_EQ(subcommand, required)        \
  do {                                                    \
    int given = bb->args->size - 1;                       \
    if (required != given)                                \
    {                                                     \
      char t[MAX_COMMAND];                                \
      mcsh_value* _v = bb->args->data[0];                 \
      size_t n = strlcpyj(t, _v->string, MAX_COMMAND);     \
      t[n] = ' ';                                         \
      strcpy(&t[n+1], subcommand);                        \
      mcsh_exception_invalid_argc(bb->status, t,          \
                                  required, given);       \
      return true;                                        \
    } } while (0);

#define EXCEPTION_ARGC_GE(required)                \
  do {                                             \
    int given = bb->args->size - 1;                \
    if (required > given)                          \
    {                                              \
      mcsh_value* _v = bb->args->data[0];          \
      char* _s = _v->string;                       \
      mcsh_exception_toofew_argc(bb->status, _s,   \
                                 required, given); \
      return true;                                 \
    } } while (0);

#define EXCEPTION_SUBARG_TYPE(subcommand, _value, required, index) \
  do {                                                             \
    if ((_value->type != required))                                \
    {                                                              \
      char t[MAX_COMMAND];                                         \
      mcsh_value* _v = bb->args->data[0];                          \
      size_t n = strlcpyj(t, _v->string, MAX_COMMAND);              \
      t[n+1] = ' ';                                                \
      strcpy(&t[n+2], subcommand);                                 \
      mcsh_exception_invalid_type(bb->status,                      \
                                  t,                               \
                                  _value, required,                \
                                  index);                          \
      return true;                                                 \
    } } while (0);

static bool
builtin_noop(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_noop: (%zi)", bb->args->size);

  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_open(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_open: (%zi)", bb->args->size);

  mcsh_value* value_filename = NULL;
  mcsh_value* value_mode     = NULL;
  char* filename;
  char* mode = "r";
  switch (bb->args->size)
  {
    case 1:
      valgrind_fail();
      break;
    case 2:
      value_filename = bb->args->data[1];
      break;
    case 3:
      value_filename = bb->args->data[1];
      value_mode     = bb->args->data[2];
      break;
    default:
      valgrind_fail();
  }
  TYPE_CHECK(value_filename, MCSH_VALUE_STRING, bb->status,
             "open", 1, "must be filename");
  filename = value_filename->string;
  show("filename: '%s'", filename);
  if (value_mode != NULL)
  {
    TYPE_CHECK(value_mode,     MCSH_VALUE_STRING, bb->status,
               "open", 2, "must be filename");
    mode = value_mode->string;
  }

  FILE* fp = fopen(filename, mode);
  if (fp == NULL)
    RAISE(bb->status, NULL, 0, "mcsh.io", "could not open: '%s': %s",
          filename, strerror(errno));
  int64_t fp_int = (int64_t) fp;
  show("fp_int: %"PRId64, fp_int);
  mcsh_value* result = mcsh_value_new_int(fp_int);
  maybe_assign(bb->output, result);
  return true;
}

static bool
get_fp(mcsh_bb* bb, mcsh_value* value,
       const char* name, int index, FILE** output)
{
  TYPE_CHECK(value, MCSH_VALUE_INT, bb->status, name, index,
             "%s requires a file pointer from open", name);
  *output = (FILE*) value->integer;
  return true;
}

static bool
builtin_close(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_close: (%zi)", bb->args->size);
  EXCEPTION_ARGC_EQ(1);

  mcsh_value* value = bb->args->data[1];

  FILE* fp = NULL;
  get_fp(bb, value, "close", 1, &fp);
  PROPAGATE(bb->status);

  int rc = fclose(fp);
  if (rc < 0)
    RAISE(bb->status, NULL, 0, "mcsh.io", "Could not close: %s",
          strerror(errno));

  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
do_write(mcsh_bb* bb, FILE* fp, size_t offset)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  buffer B;
  buffer_init(&B, bb->args->size * 8);

  if (bb->args->size > 1)
  {
    size_t i = offset;
    for ( ; i < bb->args->size - 1; i++)
    {
      mcsh_value* value = bb->args->data[i];
      mcsh_resolve(value);
      // printf("type: %i\n", value->type);
      mcsh_value_buffer(logger, value, &B);
      buffer_catc(&B, ' ');
    }
    mcsh_value* value = bb->args->data[i];
    mcsh_resolve(value);
    mcsh_value_buffer(logger, value, &B);
  }
  else
    B.data[0] = '\0';

  size_t count = fprintf(fp, "%s\n", B.data);
  mcsh_value* result = mcsh_value_new_int(count);
  maybe_assign(bb->output, result);
  buffer_finalize(&B);
  return true;
}

static bool
builtin_print(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
       "builtin_print: (%zi)", bb->args->size);
  do_write(bb, stdout, 1);
  return true;
}

static bool
builtin_write(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_write: (%zi)", bb->args->size);

  EXCEPTION_ARGC_GE(1);
  mcsh_value* value = bb->args->data[1];

  FILE* fp = NULL;
  get_fp(bb, value, "write", 1, &fp);
  PROPAGATE(bb->status);

  do_write(bb, fp, 2);

  return true;
}

static mcsh_value* read_rl(mcsh_bb* bb);
static mcsh_value* read_fgets(mcsh_bb* bb, FILE* fp);

static bool
builtin_read(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_read: (%zi)", bb->args->size);

  mcsh_value* value = NULL;
  switch (bb->args->size)
  {
    case 1:
      break;
    case 2:
      value = bb->args->data[1];
      break;
    default:
      RAISE(bb->status, NULL, 0,
            "mcsh.args", "too many args to read");
      break;
  }

  FILE* fp = NULL;
  if (value == NULL)
    fp = stdin;
  else
    get_fp(bb, value, "read", 1, &fp);
  PROPAGATE(bb->status);

  mcsh_value* result;
  if (isatty(fileno(fp)) && isatty(STDOUT_FILENO))
    result = read_rl(bb);
  else
    result = read_fgets(bb, fp);

  maybe_assign(bb->output, result);
  return true;
}

static mcsh_value*
read_rl(mcsh_bb* bb)
{
  char* text;
  mcsh_iface_get("", &text);

  mcsh_value* result = mcsh_value_new_string(bb->module->vm,
                                             trim_right(text));
  free(text);
  return result;
}

static mcsh_value*
read_fgets(mcsh_bb* bb, FILE* fp)
{
  char text[4096];
  char* s = fgets(text, 4096, fp);
  mcsh_value* result;
  if (s == NULL)
    result = &mcsh_null;
  else
    result = mcsh_value_new_string(bb->module->vm,
                                   trim_right(text));
  return result;
}

static bool
builtin_flush(mcsh_bb* bb)
{
  fflush(NULL);
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_type(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(1);
  mcsh_value* value = bb->args->data[1];
  char t[64];
  mcsh_value_type_name(value->type, t);
  mcsh_value* result =
    mcsh_value_new_string(bb->module->vm, t);
  maybe_assign(bb->output, result);
  return true;
}


static bool builtin_as_int(mcsh_bb* bb);

static bool
builtin_as(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(2);
  mcsh_value* subcommand = bb->args->data[1];
  bool rc = true;
  if (strcmp(subcommand->string, "int") == 0)
  {
    rc = builtin_as_int(bb);
  }
  else
  {
    RAISE(bb->status, NULL, 0, "mcsh.invalid_arguments",
          "unknown subcommand in builtin 'as': '%s'",
          subcommand->string);
  }
  return rc;
}

static bool
builtin_as_int(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  mcsh_value* value = bb->args->data[2];
  mcsh_resolve(value);
  EXCEPTION_SUBARG_TYPE("int", value, MCSH_VALUE_STRING, 2);
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_as_int: '%s'", value->string);
  errno = 0;
  char* p;
  long i = strtol(value->string, &p, 10);
  RAISE_IF(p == value->string || errno != 0,
           bb->status, NULL, 0,
           "mcsh.invalid_numeral",
           "could not apply 'as int' to '%s'", value->string);
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "as int result: %li", i);
  mcsh_value* result = mcsh_value_new_int(i);
  maybe_assign(bb->output, result);
  return true;
}

static bool builtin_string_encode(mcsh_bb* bb);
static bool builtin_string_decode(mcsh_bb* bb);

static bool
builtin_string(mcsh_bb* bb)
{
  if (bb->args->size <= 1)
  {
    mcsh_raise(bb->status, NULL, 0,
               "mcsh.exception.invalid_arguments",
               "string: requires a subcommand");
    return true;
  }

  mcsh_value* subcommand = bb->args->data[1];
  valgrind_assert(subcommand->type == MCSH_VALUE_STRING);
  bool rc = true;
  mcsh_log(&bb->module->vm->logger, MCSH_LOG_BUILTIN, MCSH_INFO,
           "subcommand: %s", subcommand->string);
  if (strcmp(subcommand->string, "encode") == 0)
  {
    rc = builtin_string_encode(bb);
  }
  else if (strcmp(subcommand->string, "decode") == 0)
  {
    rc = builtin_string_decode(bb);
  }
  else
  {
    mcsh_raise(bb->status, NULL, 0,
               "mcsh.exception.invalid_subcommand",
               "string: invalid subcommand: '%s'",
               subcommand->string);
    return true;
  }
  return rc;
}

static bool
builtin_string_encode(mcsh_bb* bb)
{
  EXCEPTION_SUBARGC_EQ("encode", 2);

  mcsh_value* value = bb->args->data[2];
  EXCEPTION_SUBARG_TYPE("encode", value, MCSH_VALUE_STRING, 2);

  char c = value->string[0];
  mcsh_value* result = mcsh_value_new_int(c);
  maybe_assign(bb->output, result);

  return true;
}

static bool
builtin_string_decode(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "builtin decode ...");
  EXCEPTION_SUBARGC_EQ("decode", 2);

  mcsh_value* value = bb->args->data[2];
  EXCEPTION_SUBARG_TYPE("decode", value, MCSH_VALUE_INT, 2);
  RAISE_IF(value->integer >= 256 || value->integer < 0,
           bb->status,
           NULL, 0,
           "mcsh.exception.invalid_arguments",
           "string decode: "
           "argument N must be 0 < N < 256: given: N=%i",
           value->integer);

  char t[2];
  t[0] = (char) value->integer;
  t[1] = '\0';
  mcsh_value* result = mcsh_value_new_string(bb->module->vm, t);
  maybe_assign(bb->output, result);

  return true;
}

static bool builtin_plus_string(mcsh_bb* bb);
static bool builtin_plus_list  (mcsh_bb* bb);
static bool builtin_plus_table (mcsh_bb* bb);

static bool
builtin_plus(mcsh_bb* bb)
{
  // printf("builtin_plus: (%zi)\n", bb->args->size);
  mcsh_value* target = bb->args->data[1];
  mcsh_value_type type = target->type;
  bool rc;
  switch (type)
  {
    case MCSH_VALUE_STRING:
      rc = builtin_plus_string(bb);
      break;
    case MCSH_VALUE_LIST:
      rc = builtin_plus_list(bb);
      break;
    case MCSH_VALUE_TABLE:
      rc = builtin_plus_table(bb);
      break;
    default:
      fail("builtin_plus: bad type!");
  }

  // Assume subroutine assigned to bb->result
  return rc;
}

static bool
builtin_plus_string(mcsh_bb* bb)
{
  buffer B;
  buffer_init(&B, bb->args->size * 4);
  char t[1024];
  for (size_t i = 1; i < bb->args->size; i++)
  {
    mcsh_value* value = bb->args->data[i];
    mcsh_resolve(value);
    mcsh_to_string(&bb->module->vm->logger, t, 1024, value);
    buffer_cat(&B, t);
  }
  mcsh_value* result = mcsh_value_new_string_null();
  result->string = B.data;
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_plus_list(mcsh_bb* bb)
{
  mcsh_value* target = bb->args->data[1];
  list_array* L = target->list;
  for (size_t i = 2; i < bb->args->size; i++)
  {
    mcsh_value_grab(&bb->module->vm->logger, bb->args->data[i]);
    list_array_add(L, bb->args->data[i]);
  }
  maybe_assign(bb->output, target);
  // printf("new list size: %zi\n", L->size);
  return true;
}

static bool
builtin_plus_table(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(3);
  mcsh_value* target = bb->args->data[1];
  mcsh_value* key    = bb->args->data[2];
  mcsh_value* value  = bb->args->data[3];
  mcsh_table_add(&bb->module->vm->logger,
                 target, key, value);
  maybe_assign(bb->output, target);
  // printf("new table size: %i\n", target->table->size);
  return true;
}

static bool
builtin_incr(mcsh_bb* bb)
{
  mcsh_log(&bb->module->vm->logger, MCSH_LOG_BUILTIN, MCSH_INFO,
           "builtin incr ...");

  mcsh_value* target = bb->args->data[1];
  valgrind_assert(target->type == MCSH_VALUE_STRING);
  char* name = target->string;
  mcsh_value* v;
  mcsh_stack_search(bb->module->vm->stack.current, name, &v);

  int64_t i;
  bool rc = mcsh_value_integer(v, &i);
  CHECK(rc, "builtin_incr: not integer");
  i++;
  mcsh_value* new = mcsh_value_new_int(i);
  mcsh_set_value(bb->module, name, new, bb->status);
  // TODO: check status
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_exit(mcsh_bb* bb)
{
  int code = EXIT_SUCCESS;
  if (bb->args->size > 1)
  {
    mcsh_value* value = bb->args->data[1];
    mcsh_resolve(value);
    int64_t code;
    bool rc = mcsh_value_integer(value, &code);
    CHECK(rc, "builtin_exit: not integer");
    mcsh_log(&bb->module->vm->logger, MCSH_LOG_BUILTIN, MCSH_INFO,
             "builtin exit: code=%"PRId64, code);
  }
  *bb->output = &mcsh_null;
  bb->status->code  = MCSH_EXIT;
  bb->status->value = mcsh_value_new_int(code);
  return true;
}

static bool
builtin_expr(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  buffer B;
  buffer_init(&B, bb->args->size * 4);
  LOG(MCSH_LOG_BUILTIN, MCSH_DEBUG,
           "builtin: expr: (%zi)", bb->args->size);
  for (size_t i = 1; i < bb->args->size; i++)
  {
    mcsh_value* value = bb->args->data[i];
    mcsh_resolve(value);
    const int length = 64;
    char t[length];
    mcsh_to_string(logger, t, length, value);
    buffer_cat (&B, t);
    buffer_catc(&B, ' ');
  }
  buffer_catc(&B, '\n');
  // printf("builtin_expr string: '%s'\n", B.data);
  buffer_cat(&B, "\n");

  mcsh_value* result;
  mcsh_node* node;
  mcsh_expr_scan(B.data, &node, bb->status);
  // printf("scan ok.\n");

  mcsh_expr* expr;
  mcsh_node_to_expr(node, &expr);
  // printf("translate OK\n");

  // mcsh_expr_print(expr, 0);

  bool rc = mcsh_expr_eval(bb->module->vm, expr, &result);
  // printf("execute\n");
  CHECK(rc, "mcsh: expr execution failed!\n");

  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_set(mcsh_bb* bb)
{
  mcsh_value* target = bb->args->data[1];
  buffer b;
  buffer_init(&b, 16);
  // mcsh_value_buffer
  valgrind_assert_msg(target->type == MCSH_VALUE_STRING,
                      "type: %i", target->type);
  char* name = target->string;
  mcsh_value* value = bb->args->data[2];
  // printf("set type: %i\n", value->type);
  // mcsh_value* clone = mcsh_value_clone(value);
  /* mcsh_log(&bb->module->vm->logger, MCSH_LOG_DATA, MCSH_LOG_INFO, */
  /*          "clone: %p"); */
  bool rc = mcsh_set_value(bb->module, name, value, bb->status);
  // TODO: check status
  // printf("value: %p\n", value);
  maybe_assign(bb->output, value);
  // printf("out type: %i\n", (*(bb->output))->type);
  return rc;
}

static bool
builtin_drop(mcsh_bb* bb)
{
  mcsh_value* target = bb->args->data[1];
  buffer b;
  buffer_init(&b, 16);
  // mcsh_value_buffer
  valgrind_assert_msg(target->type == MCSH_VALUE_STRING,
                      "type: %i", target->type);
  char* name = target->string;
  bool rc = mcsh_drop_variable(bb->module, name, bb->status);
  return rc;
}

static void link_to_global(mcsh_module* module,
                           const char* name, mcsh_value* global);

static bool
builtin_global(mcsh_bb* bb)
{
  printf("builtin_global: (%zi)\n", bb->args->size);
  mcsh_value* target = bb->args->data[1];
  mcsh_resolve(target);
  valgrind_assert(target->type == MCSH_VALUE_STRING);
  char* name = target->string;
  mcsh_module* module = bb->module;
  mcsh_value* global;
  if (table_search(&module->vm->globals, name, (void*) &global))
  {
    printf("link to existing global: '%s'\n", name);
  }
  else
  {
    global = mcsh_value_new_null();
    printf("new global: '%s'\n", name);
    table_add(&module->vm->globals, name, global);
  }
  link_to_global(module, name, global);
  maybe_assign(bb->output, global);
  return true;
}

static void
link_to_global(mcsh_module* module,
               const char* name, mcsh_value* global)
{

  mcsh_value* value = mcsh_value_new_link(global);
  strmap_add(&module->vm->stack.current->vars, name, value);
}

static void link_to_public(mcsh_module* module,
                           const char* name, mcsh_value* public);

static bool
builtin_public(mcsh_bb* bb)
{
  mcsh_log(&bb->module->vm->logger, MCSH_LOG_BUILTIN, MCSH_DEBUG,
           "builtin_public: (%zi)", bb->args->size);
  mcsh_value* target = bb->args->data[1];
  mcsh_resolve(target);
  valgrind_assert(target->type == MCSH_VALUE_STRING);
  char* name = target->string;
  mcsh_module* module = bb->module;
  mcsh_value* public;
  if (strmap_search(&module->vars, name, (void*) &public))
  {
    printf("link to existing public: '%s'\n", name);
  }
  else
  {
    public = mcsh_value_new_null();
    mcsh_log(&bb->module->vm->logger, MCSH_LOG_BUILTIN, MCSH_DEBUG,
             "new public: '%s'\n", name);
    strmap_add(&module->vars, name, public);
  }
  link_to_public(module, name, public);
  maybe_assign(bb->output, public);
  return true;
}

static void
link_to_public(mcsh_module* module,
               const char* name, mcsh_value* public)
{
  mcsh_value* value = malloc(sizeof(mcsh_value));
  value->type = MCSH_VALUE_LINK;
  value->link = public;
  strmap_add(&module->vm->stack.current->vars, name, value);
}

static bool
builtin_signature(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_signature: (%zi) argc=%u\n",
      bb->args->size, bb->module->vm->argc);
  mcsh_module* module = bb->module;
  mcsh_vm*     vm     = module->vm;
  for (size_t i = 1; i < bb->args->size; i++)
  {
    mcsh_value* name = bb->args->data[i];
    mcsh_resolve(name);
    LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "assign to %s\n", name->string);
    if (i >= vm->argc)
      RAISE(bb->status, NULL, 0, "mcsh.exception.index_error",
            "signature: could not assign to %s, "
            "too few arguments (%i)", name->string, vm->argc);
    mcsh_value* global = mcsh_value_new_string(vm, vm->argv[i]);
    mcsh_value* old;
    if (table_set(&module->vm->globals, name->string, global,
                  (void*) &old))
    {} // TODO: clean up old value
    else
    {
      LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
          "signature: set new global: '%s'\n", name->string);
    }
    link_to_global(module, name->string, global);
  }
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static inline mcsh_fn_type fn_string2type(mcsh_logger* logger,
                                          const char* name);

static bool
builtin_function(mcsh_bb* bb)
{
  mcsh_value* v0 = bb->args->data[0];
  valgrind_assert(v0->type == MCSH_VALUE_STRING);
  char* this = v0->string;

  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin_%s: (%zi)", this, bb->args->size);
  mcsh_fn_type type = fn_string2type(logger, this);

  valgrind_assert_msg(bb->args->size == 4, "simple function plz!");

  mcsh_value* name = bb->args->data[1];
  valgrind_assert_msg(name->type == MCSH_VALUE_STRING,
                      "function name must be a string!");
  // sgtokens: signature list as unprocessed text tokens
  mcsh_value* sgtokens = bb->args->data[2];
  valgrind_assert_msg(sgtokens->type == MCSH_VALUE_BLOCK,
                      "function signature must be a block!");
  mcsh_value* body = bb->args->data[3];
  valgrind_assert_msg(body->type == MCSH_VALUE_BLOCK,
                      "function body must be a block!");

  mcsh_value* f = mcsh_value_new_function(bb->module,
                                          type,
                                          name->string,
                                          sgtokens->block,
                                          body->block,
                                          bb->status);
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "CONSTRUCTED %s: %p %p %i",
      this, f, f->function, f->function->type);
  if (mcsh_log_check(logger, MCSH_LOG_BUILTIN, MCSH_INFO))
    mcsh_signature_print(&f->function->signature);
  mcsh_set_value(bb->module, name->string, f, bb->status);
  // TODO: check status

  return true;
}

static inline mcsh_fn_type
fn_string2type(mcsh_logger* logger, const char* name)
{
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "name: '%s'", name);
  lookup_entry L[4] =
    {{MCSH_FN_NORMAL,  "function"},
     {MCSH_FN_INPLACE, "inplace"},
     {MCSH_FN_MACRO,   "macro"},
     lookup_sentinel
    };
  int t = lookup_by_text(L, name);
  valgrind_assert(t >= 0);
  mcsh_fn_type result = t;
  return result;
}

static bool builtin_os_env(mcsh_bb* bb);
static bool builtin_os_pwd(mcsh_bb* bb);
static bool builtin_os_cd (mcsh_bb* bb);
static bool builtin_os_pid(mcsh_bb* bb);
static bool builtin_os_dirname(mcsh_bb* bb);
static bool builtin_os_basename(mcsh_bb* bb);
static bool builtin_os_resolve(mcsh_bb* bb);

static bool
builtin_os(mcsh_bb* bb)
{
  if (bb->args->size <= 1)
  {
    valgrind_fail();
  }

  mcsh_value* subcommand = bb->args->data[1];
  valgrind_assert(subcommand->type == MCSH_VALUE_STRING);

  bool rc;
  if (strcmp(subcommand->string, "env") == 0)
    rc = builtin_os_env(bb);
  else if (strcmp(subcommand->string, "pwd") == 0)
    rc = builtin_os_pwd(bb);
  else if (strcmp(subcommand->string, "cd") == 0)
    rc = builtin_os_cd(bb);
  else if (strcmp(subcommand->string, "pid") == 0)
    rc = builtin_os_pid(bb);
  else if (strcmp(subcommand->string, "..") == 0)
    rc = builtin_os_dirname(bb);
  else if (strcmp(subcommand->string, "file") == 0)
    rc = builtin_os_basename(bb);
  else if (strcmp(subcommand->string, "resolve") == 0)
    rc = builtin_os_resolve(bb);

  else
    RAISE(bb->status, NULL, 0, "mcsh.exception.invalid_arguments",
          "builtin os: unknown subcommand: '%s'", subcommand->string);

  CHECK(rc, "builtin os failed");

  return true;
}

static bool
builtin_os_env(mcsh_bb* bb)
{
  printf("getcwd: %zi\n", bb->args->size);
  mcsh_value* key = bb->args->data[2];
  mcsh_resolve(key);
  valgrind_assert(key->type == MCSH_VALUE_STRING);
  char* s = getenv(key->string);
  valgrind_assert(s != NULL);
  mcsh_value* result = mcsh_value_new_string(bb->module->vm, s);
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_os_pwd(mcsh_bb* bb)
{
  // valgrind_assert(args->size == 1);
  printf("getcwd: %zi\n", bb->args->size);
  char p[PATH_MAX];
  char* t = getcwd(p, PATH_MAX);
  if (t == NULL)
  {
    printf("error in getcwd\n");
    perror("mcsh:");
    return false;
  }
  printf("getcwd: %s\n", p);
  mcsh_value* result = mcsh_value_new_string(bb->module->vm, p);
  *bb->output = result;
  return true;
}

static bool
builtin_os_cd(mcsh_bb* bb)
{
  valgrind_assert(bb->args->size == 3);
  mcsh_value* path = bb->args->data[2];
  mcsh_resolve(path);
  int rc = chdir(path->string);
  if (rc != 0)
    perror("mcsh");
  *bb->output = &mcsh_null;
  return true;
}

static bool
builtin_os_pid(mcsh_bb* bb)
{
  valgrind_assert(bb->args->size == 2);
  pid_t pid = getpid();
  *bb->output = mcsh_value_new_int(pid);
  return true;
}

static bool
builtin_os_dirname(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(2);
  mcsh_value* a = bb->args->data[2];
  char t[PATH_MAX];
  strcpy(t, a->string);
  parent(t);
  *bb->output = mcsh_value_new_string(bb->module->vm, t);
  return true;
}

static bool
builtin_os_basename(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(2);
  mcsh_value* a = bb->args->data[2];
  char t[PATH_MAX];
  strcpy(t, a->string);
  char* b = basename(t);
  *bb->output = mcsh_value_new_string(bb->module->vm, b);
  return true;
}

static bool
builtin_os_resolve(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(2);
  mcsh_value* a = bb->args->data[2];
  char t[PATH_MAX];
  strcpy(t, a->string);
  char b[PATH_MAX];
  char* p = realpath(t, b);
  if (p == NULL)
    RAISE(bb->status, NULL, 0, "mcsh.path_error",
          "could not resolve: '%s' %s", t, strerror(errno));
  *bb->output = mcsh_value_new_string(bb->module->vm, b);
  return true;
}

static bool
builtin_sh(mcsh_bb* bb)
{
  valgrind_assert(bb->args->size > 1);
  printf("sh: %zi\n", bb->args->size);
  // Replace arg 0:
  bb->args->data[0] =
    mcsh_value_new_string(bb->module->vm, "sh -c '");
  // Append end quote to args...
  mcsh_value end_quote;
  mcsh_value_init_string(&end_quote, "'");
  list_array_add(bb->args, &end_quote);
  char* cmd = list_array_join_values(bb->args, " ");
  printf("sh: %s\n", cmd);
  int rc = system(cmd);
  char exitcode[8];
  sprintf(exitcode, "%i", rc);
  mcsh_value* result =
    mcsh_value_new_string(bb->module->vm, exitcode);
  free(cmd);
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_list_create(mcsh_bb* bb)
{
  mcsh_value* result = mcsh_value_new_list(bb->module->vm);
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_table_create(mcsh_bb* bb)
{
  mcsh_value* result = mcsh_value_new_table(bb->module->vm, 8);
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_get(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;

  mcsh_value* container = bb->args->data[1];
  mcsh_value* index = bb->args->data[2];
  mcsh_resolve(index);

  LOG(MCSH_LOG_BUILTIN, MCSH_INFO,
      "builtin get: %i %p\n", container->type, container);

  switch (container->type)
  {
    case MCSH_VALUE_LIST:
      int64_t i;
      mcsh_value_integer(index, &i);
      LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "GET: index=%"PRId64"\n", i);
      list_array* L = container->list;
      CHECK(i >= 0, "get(): index < 0");
      CHECK((uint64_t) i < L->size, "get(): index too big");
      mcsh_value* result = L->data[i];
      maybe_assign(bb->output, result);
      break;
    default:
      printf("get(): given invalid container value.\n");
      return false;
  }
  return true;
}

static bool
builtin_split(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_DEBUG, "split...");
  mcsh_value* result = mcsh_value_new_list(bb->module->vm);
  list_array* L = result->list;
  mcsh_value* target    = bb->args->data[1];
  mcsh_value* delimiter = bb->args->data[2];
  mcsh_resolve(target);
  mcsh_resolve(delimiter);
  valgrind_assert(target   ->type == MCSH_VALUE_STRING);
  valgrind_assert(delimiter->type == MCSH_VALUE_STRING);
  char* s = target->string;
  char* d = delimiter->string;
  char* t = alloca(strlen(s+1));  // Temporary space
  char* p = s;  // Moving start pointer through target s
  char* q;      // Next match
  int count = 0;
  mcsh_value* value;
  while (true)
  {
    q = strstr(p, d);
    if (q == NULL) break;
    strlcpyj(t, p, q-p+1);
    LOG(MCSH_LOG_BUILTIN, MCSH_DEBUG, "%i: '%s'", count, t);
    value = mcsh_value_new_string(bb->module->vm, t);
    list_array_add(L, value);
    mcsh_value_grab(logger, value);
    p = q+1;
    count++;
  }
  if (strlen(p) > 0)
  {
    LOG(MCSH_LOG_BUILTIN, MCSH_DEBUG, "%i: '%s'", count, p);
    value = mcsh_value_new_string(bb->module->vm, p);
    list_array_add(L, value);
    mcsh_value_grab(logger, value);
  }

  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_join(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  LOG(MCSH_LOG_BUILTIN, MCSH_INFO, "JOIN");
  mcsh_value* target    = bb->args->data[1];
  mcsh_value* delimiter = bb->args->data[2];
  mcsh_resolve(target);
  mcsh_resolve(delimiter);
  valgrind_assert(target   ->type == MCSH_VALUE_LIST);
  valgrind_assert(delimiter->type == MCSH_VALUE_STRING);
  list_array* L = target->list;
  char* d = delimiter->string;
  buffer B;
  buffer_init(&B, L->size);
  mcsh_join_list_to_buffer(logger, L, d, &B);
  mcsh_value* result =
    mcsh_value_new_string(bb->module->vm, B.data);
  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_find(mcsh_bb* bb)
{
  // printf("FIND\n");
  mcsh_value* haystack = bb->args->data[1];
  mcsh_resolve(haystack);
  mcsh_value* needle = bb->args->data[2];
  mcsh_resolve(needle);
  char* h = haystack->string;
  char* n = needle  ->string;
  char* p = strstr(h, n);
  mcsh_value* result;
  if (p == NULL)
  {
    result = mcsh_value_new_int(-1);
  }
  else
  {
    size_t z = p - h;
    // printf("found: %zi\n", z);
    result = mcsh_value_new_int(z);
  }
  maybe_assign(bb->output, result);
  bb->status->code = MCSH_OK;
  return true;
}

static bool
builtin_substring(mcsh_bb* bb)
{
  printf("SUBSTRING\n");
  mcsh_value* target = bb->args->data[1];
  mcsh_resolve(target);
  printf("arg 1 %p\n", target);
  mcsh_value* start = bb->args->data[2];
  mcsh_resolve(start);
  mcsh_value* end = bb->args->data[3];
  mcsh_resolve(end);

  char* t = target->string;
  int64_t p0, p1;
  mcsh_value_integer(start, &p0);
  mcsh_value_integer(end,   &p1);
  mcsh_value* result;
  if (p1 < p0) p1 = p0;
  int r_length = p1-p0+1;
  char r[r_length];
  strlcpyj(r, t+p0, r_length);
  printf("result: '%s'\n", r);
  result = mcsh_value_new_string(bb->module->vm, r);

  maybe_assign(bb->output, result);
  bb->status->code = MCSH_OK;
  return true;
}

static bool
builtin_import(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(1);
  mcsh_value* target = bb->args->data[1];
  mcsh_resolve(target);
  valgrind_assert(target->type == MCSH_VALUE_STRING);
  char* t = target->string;
  mcsh_import(bb->module, t, bb->output, bb->status);
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_source(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(1);
  mcsh_value* target = bb->args->data[1];
  mcsh_resolve(target);
  valgrind_assert(target->type == MCSH_VALUE_STRING);
  char* t = target->string;
  mcsh_source(bb->module, t, bb->output, bb->status);
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_eval(mcsh_bb* bb)
{
  buffer b;
  buffer_init(&b, bb->args->size * 32);
  for (size_t i = 1; i < bb->args->size; i++)
  {
    mcsh_value* target = bb->args->data[i];
    mcsh_resolve(target);
    valgrind_assert(target->type == MCSH_VALUE_STRING);
    char* t = target->string;
    buffer_cat(&b, t);
    buffer_catc(&b, ' ');
  }

  mcsh_eval(bb->module, b.data, bb->output, bb->status);
  buffer_finalize(&b);
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_break(mcsh_bb* bb)
{
  bb->status->code = MCSH_BREAK;
  return true;
}

static bool
builtin_continue(mcsh_bb* bb)
{
  bb->status->code = MCSH_CONTINUE;
  return true;
}

static bool
builtin_sleep(mcsh_bb* bb)
{
  printf("c: '%s'\n", (char*) bb->args->data[0]);
  EXCEPTION_ARGC_EQ(1);
  mcsh_value* value = bb->args->data[1];
  mcsh_resolve(value);
  double d;
  bool b = mcsh_to_float(&d, value, bb->status);
  valgrind_assert(b);
  if (bb->status->code != MCSH_OK) return true;

  int    wholes   = (int) d;
  double fraction = d - (double) wholes;
  int    nanos    = (int) (fraction * 1000 * 1000 * 1000);  // 10^9
  struct timespec t;
  t.tv_sec  = wholes;
  t.tv_nsec = nanos;
  nanosleep(&t, NULL);
  maybe_assign(bb->output, &mcsh_null);
  return true;
}

static bool
builtin_clock(mcsh_bb* bb)
{
  /* ARG_CHECK(1); */
  /* mcsh_value* value = bb->args->data[1]; */
  /* mcsh_resolve(value); */

  time_t t = time(NULL);
  mcsh_value* result = mcsh_value_new_int(t);

  maybe_assign(bb->output, result);
  return true;
}

static bool
builtin_bang(mcsh_bb* bb)
{
  mcsh_vm* vm = bb->module->vm;
  mcsh_logger* logger = &vm->logger;

  int n = bb->args->size;
  char** a = malloc(sizeof(char*) * n);
  buffer B;
  buffer_init(&B, 32);
  mcsh_value_buffer(logger, bb->args->data[1], &B);
  char* cmd = buffer_dup(&B);
  show("cmd: %s", cmd);
  int i = 2;
  a[0] = cmd;
  for ( ; i < n; i++)
  {
    buffer_reset(&B);
    mcsh_value_buffer(logger, bb->args->data[i], &B);
    a[i-1] = buffer_dup(&B);
    show("%i %s", i-1, a[i-1]);
  }
  a[i-1] = NULL;

  logger->show_pid = true;

  mcsh_exec(bb->module,
            cmd,
            a,
            bb->output,
            bb->status);

  return true;
}

static bool
builtin_bg(mcsh_bb* bb)
{
  mcsh_logger* logger = &bb->module->vm->logger;
  mcsh_stmts stmts;
  mcsh_stmt stmt;
  stmt.module = bb->module;
  stmt.parent = NULL;
  list_array_init(&stmt.things, bb->args->size);
  stmt.line = 0;
  list_array_init(&stmts.stmts, 1);
  list_array_add(&stmts.stmts, &stmt);
  for (size_t i = 1; i < bb->args->size; i++)
  {
    mcsh_thing* thing = mcsh_thing_from_value(bb->module,
                                              bb->args->data[i]);
    list_array_add(&stmt.things, thing);
  }

  logger->show_pid = true;

  mcsh_bg(bb->module,
          &stmts,
          bb->output,
          bb->status);

  mcsh_value* pidv = *bb->output;
  pid_t pid = pidv->integer;

  list_i_add(&bb->module->vm->jobs, pid);

  LOG(MCSH_LOG_BUILTIN, MCSH_WARN, "mcsh_bg: done.");

  // free

  return true;
}

static bool
builtin_wait(mcsh_bb* bb)
{
  // ARG_CHECK(1);
  EXCEPTION_ARGC_EQ(0)
  mcsh_value* value = bb->args->data[1];
  mcsh_resolve(value);
  pid_t pid;
  bool rc = mcsh_value_integer(value, (int64_t*) &pid);
  CHECK(rc, "not an integer");
  int wstatus;
  waitpid(pid, &wstatus, 0);

  mcsh_value* result = mcsh_value_new_int(wstatus);
  maybe_assign(bb->output, result);
  bb->status->code = MCSH_OK;
  return true;
}

static bool
builtin_jobs(mcsh_bb* bb)
{
  EXCEPTION_ARGC_EQ(0);
  int i = 0;
  for (struct list_i_item* item = bb->module->vm->jobs.head;
       item != NULL; item = item->next)
    printf("[%i] %6i\n", i++, item->data);

  return true;
}

static void
builtins_add()
{
  table_add(mcsh.builtins, ":",         builtin_noop);
  table_add(mcsh.builtins, "+",         builtin_plus);
  table_add(mcsh.builtins, "++",        builtin_incr);
  table_add(mcsh.builtins, "$",         builtin_expr);
  table_add(mcsh.builtins, "=",         builtin_set);
  table_add(mcsh.builtins, "drop",      builtin_drop);
  table_add(mcsh.builtins, "global",    builtin_global);
  table_add(mcsh.builtins, "public",    builtin_public);
  table_add(mcsh.builtins, "signature", builtin_signature);
  table_add(mcsh.builtins, "function",  builtin_function);
  table_add(mcsh.builtins, "inplace",   builtin_function);
  table_add(mcsh.builtins, "macro",     builtin_function);
  table_add(mcsh.builtins, "open",      builtin_open);
  table_add(mcsh.builtins, "close",     builtin_close);
  table_add(mcsh.builtins, "print",     builtin_print);
  table_add(mcsh.builtins, "<<",        builtin_read);
  table_add(mcsh.builtins, ">>",        builtin_write);
  table_add(mcsh.builtins, "flush",     builtin_flush);
  table_add(mcsh.builtins, "type",      builtin_type);
  table_add(mcsh.builtins, "as",        builtin_as);
  table_add(mcsh.builtins, "string",    builtin_string);
  table_add(mcsh.builtins, "find",      builtin_find);
  table_add(mcsh.builtins, "substring", builtin_substring);
  table_add(mcsh.builtins, "sh",        builtin_sh);
  table_add(mcsh.builtins, "list",      builtin_list_create);
  table_add(mcsh.builtins, "table",     builtin_table_create);
  table_add(mcsh.builtins, "get",       builtin_get);
  table_add(mcsh.builtins, "split",     builtin_split);
  table_add(mcsh.builtins, "join",      builtin_join);
  table_add(mcsh.builtins, "import",    builtin_import);
  table_add(mcsh.builtins, ".",         builtin_source);
  table_add(mcsh.builtins, "eval",      builtin_eval);
  table_add(mcsh.builtins, "break",     builtin_break);
  table_add(mcsh.builtins, "continue",  builtin_continue);
  table_add(mcsh.builtins, "os",        builtin_os);
  table_add(mcsh.builtins, "sleep",     builtin_sleep);
  table_add(mcsh.builtins, "clock",     builtin_clock);
  table_add(mcsh.builtins, "!",         builtin_bang);
  table_add(mcsh.builtins, "bg",        builtin_bg);
  table_add(mcsh.builtins, "wait",      builtin_wait);
  table_add(mcsh.builtins, "jobs",      builtin_jobs);
  table_add(mcsh.builtins, "exit",      builtin_exit);
}

void
mcsh_builtins_finalize()
{
  table_free(mcsh.builtins);
}
