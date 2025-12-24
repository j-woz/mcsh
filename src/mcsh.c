
/**
   MCSH C
*/

#define _GNU_SOURCE // for asprintf(), vasprintf()
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "strlcpyj.h"

#include "lookup.h"

#include "mcsh.h"
#include "exceptions.h"
#include "builtins.h"
#include "mcsh-sys.h"

#include "mcsh-expr-parser.h"

// For mcsh-script.l
extern int mcsh_script_parse(void);
extern int mcsh_script_lex_destroy(void);

// For mcsh-expr.l
extern int mcsh_expr_parse(void);

mcsh_system mcsh;

// Apparent duplicate in GCC 10.3.0 2021-08-11:
// bool mcsh_script_token_quoted;

/** Contains mcsh_things - TODO: remove this */
static list_array terms_in;

int mcsh_script_get_lineno(void);

mcsh_value mcsh_null;
char mcsh_null_string[] = "mcsh.NULL";

/** Counter for miscellaneous identifiers */
uint64_t counter = 1;

/** thing->type as string */
static void
thing_str(mcsh_thing* thing, char* output)
{
  if (thing == NULL)
  {
    sprintf(output, "NULL");
    return;
  }

  lookup_entry L[8] =
    {{MCSH_THING_NULL,   "NULL"  },
     {MCSH_THING_TOKEN,  "TOKEN" },
     {MCSH_THING_STMT,   "STMT"  },
     {MCSH_THING_BLOCK,  "BLOCK" },
     {MCSH_THING_SUBCMD, "SUBCMD"},
     {MCSH_THING_SUBFUN, "SUBFUN"},
     {MCSH_THING_MODULE, "MODULE"},
     lookup_sentinel
    };
  char* t = lookup_by_code(L, thing->type);
  valgrind_assert_msg(t != NULL,
                      "thing_str(): unknown thing->type: %i\n",
                      thing->type);
  strcpy(output, t);
}

static bool system_init(mcsh_system* state);

bool
mcsh_init()
{
  bool rc = system_init(&mcsh);
  if (!rc) return false;
  mcsh_builtins_init();
  list_array_init(&terms_in, 16);
  mcsh_null.type = MCSH_VALUE_STRING;
  mcsh_null.string = mcsh_null_string;
  return true;
}

void
mcsh_help()
{
  printf("mcsh: usage:                        \n"
         "      -h         help               \n"
         "      -c CMD     run command string \n"
         );
}

void
mcsh_status_init(mcsh_status* status)
{
  status->code      = MCSH_PROTO;
  status->exception = NULL;
  status->value     = NULL;
}

/**
   Note: This transfers ownership of the cmd strings to the
         cmd_line argv
*/
void
mcsh_handle_cmd_string(list_array* cmd_tmp, mcsh_cmd_line* cmd)
{
  if (cmd_tmp->size == 0) return;

  // User requested strings with -c
  cmd->mode = MCSH_MODE_STRING;
  cmd->argc = cmd_tmp->size;
  cmd->argv = malloc_checked(sizeof(char*) * (cmd_tmp->size+1));
  size_t i = 0;
  for ( ; i < cmd_tmp->size; i++)
  {
    cmd->argv[i] = cmd_tmp->data[i];
  }
  cmd->argv[i] = NULL;
}

bool
mcsh_parse_options(unsigned int argc, char* argv[],
                   mcsh_cmd_line* cmd)
{
  list_array cmd_tmp;
  list_array_init(&cmd_tmp, 0);
  while (true)
  {
    int c = getopt(argc, argv, "c:h");
    if (c == -1) break;
    switch (c)
    {
      case 'h':
        mcsh_help();
        exit(EXIT_SUCCESS);
      case 'c':
        list_array_add(&cmd_tmp, strdup(optarg));
        break;
      default:
        fail("unknown flag: %c\n", c);
    }
  }
  mcsh_handle_cmd_string(&cmd_tmp, cmd);
  list_array_finalize(&cmd_tmp);
  return true;
}

/** Handle K1=V1 K2=V2 script.mc arg1 arg2 ,
    producing a cmd_line
 */
bool
mcsh_parse_args(unsigned int argc, char* argv[],
                mcsh_cmd_line* cmd)
{
  unsigned int index = 1;
  char key[1024];
  strcpy(cmd->mcsh_command, argv[0]);
  for ( ; index < argc; index++)
  {
    char* p;
    if ((p = strstr(argv[index], "=")))
    {
      size_t key_length = p - argv[index];
      strlcpyj(key, argv[index], key_length+1);
      printf("key: '%s'\n", key);
      char* value = strdup(argv[index] + key_length+1);
      printf("value: '%s'\n", value);
      strmap_add(&cmd->globals, key, value);
    }
    else break;
  }

  cmd->argc = argc - index;
  if (argc - index > 0)
  {
    /* printf("argc:  %u\n", argc); */
    /* printf("index: %u\n", index); */
    cmd->argv = calloc(cmd->argc, sizeof(char*));
    char* script = argv[index];
    cmd->argv[0] = script;
    FILE* fp = fopen(script, "r");
    if (fp == NULL)
    {
      printf("mcsh: could not run '%s'\n", script);
      printf("mcsh: %s\n", strerror(errno));
      return false;
    }
    cmd->mode = MCSH_MODE_SCRIPT;
    cmd->stream = fp;
  }
  else
  {
    // printf("using stdin\n");
    cmd->argv = calloc(1, sizeof(char*));
    cmd->stream = stdin;
    cmd->mode = MCSH_MODE_INTERACTIVE;
    cmd->argv[0] = "stdin";
  }

  // The start of the script args:
  unsigned int arg_offset = index;
  // printf("arg_offset: %u\n", arg_offset);
  for (int i = 1; arg_offset + i < argc; i++)
  {
    // printf("argv %u = '%s'\n", i, argv[arg_offset+i]);
    cmd->argv[i] = argv[arg_offset+i];
  }
  return true;
}

static void parse_state_init(mcsh_parse_state* parse_state);

static bool
system_init(mcsh_system* sys)
{
  sys->pid = getpid();
  bool rc = mcsh_log_init(&sys->logger, sys->pid, MCSH_WARN);
  mcsh_log(&sys->logger, MCSH_LOG_SYSTEM, MCSH_DEBUG,
           "PID: %i", sys->pid);
  sys->vm_count = 0;
  parse_state_init(&sys->parse_state);
  sys->vm_capacity = 4;
  sys->vms = calloc_checked(sizeof(mcsh_vm*), sys->vm_capacity);
  return rc;
}

void vm_add(mcsh_vm* vm);

void
mcsh_vm_init(mcsh_vm* vm)
{
  vm_add(vm);
  mcsh_log_init(&vm->logger, mcsh.pid, MCSH_INFO);
  table_init(&vm->globals, 128);
  vm->main = malloc_checked(sizeof(mcsh_module));
  list_array_init(&vm->path, 4);
  mcsh_module_init(vm->main, vm);
  mcsh_entry* entry = malloc_checked(sizeof(mcsh_entry));
  vm->entry_main = entry;
  mcsh_entry_init_module(entry, vm->main, NULL);
  mcsh_stack_init(&vm->stack, vm);
  vm->stack.current = entry;
  mcsh_data_init(vm);
  list_i_init(&vm->jobs);
  vm->exit_code_last = 0;
}

void
vm_add(mcsh_vm* vm)
{
  size_t i;
  for (i = 0; i < mcsh.vm_capacity; i++)
  {
    if (mcsh.vms[i] == NULL)
    {
      goto done;
    }
  }
  // No slot for a VM!
  mcsh.vm_capacity = mcsh.vm_capacity * 2;
  mcsh.vms = realloc_checked(mcsh.vms,
                             mcsh.vm_count * sizeof(mcsh_vm*));
  done:
  vm->id = i;
  mcsh.vms[vm->id] = vm;
  mcsh.vm_count++;
  mcsh_log(&mcsh.logger, MCSH_LOG_SYSTEM, MCSH_DEBUG,
           "VM ID: %zi", vm->id);
}

void
mcsh_cmd_line_init(mcsh_cmd_line* cmd)
{
  cmd->mode = MCSH_MODE_PROTO;
  strmap_init(&cmd->globals, 4);
}

void
mcsh_cmd_line_finalize(mcsh_cmd_line* cmd)
{
  strmap_finalize(&cmd->globals);
}

mcsh_value* mcsh_value_new_list_charppc(mcsh_vm* vm,
                                              int argc, char** argv);

static bool vm_init_path(mcsh_vm* vm);

void
mcsh_vm_init_argv(mcsh_vm* vm, int argc, char** argv)
{
  mcsh_vm_init(vm);
  mcsh_log(&vm->logger, MCSH_LOG_CORE, MCSH_INFO,
           "vm_init_argv: argc=%i", argc);
  strmap parameters;
  strmap_init(&parameters, 4);
  mcsh_module* module_mcsh = malloc(sizeof(*module_mcsh));
  mcsh_module_init(module_mcsh, vm);
  mcsh_value* module_mcsh_value = mcsh_value_new_module(vm, module_mcsh);
  /* strmap_add(&parameters, "mcsh.argc", mcsh_value_construct_int(argc)); */
  /* strmap_add(&parameters, "mcsh.argv", */
  /*            mcsh_value_construct_list_charppc(vm, argc, argv)); */
  strmap_add(&module_mcsh_value->module->vars,
             "argc", mcsh_value_new_int(argc));
  strmap_add(&module_mcsh_value->module->vars,
             "argv" , mcsh_value_new_list_charppc(vm, argc, argv));

  strmap_add(&parameters, "mcsh", module_mcsh);
  mcsh_assign_specials(vm, &parameters);
}

static void add_globals(mcsh_vm* vm, strmap* map);

void
mcsh_vm_init_cmd(mcsh_vm* vm, mcsh_cmd_line* cmd)
{
  mcsh_vm_init(vm);
  mcsh_log(&vm->logger, MCSH_LOG_SYSTEM, MCSH_INFO,
           "vm_init_cmd: argc=%u", cmd->argc);
  // TODO: dup these:
  vm->argc = cmd->argc;
  vm->argv = cmd->argv;
  strmap parameters;
  strmap_init(&parameters, 4);

  mcsh_module* module_mcsh = malloc(sizeof(*module_mcsh));
  mcsh_module_init(module_mcsh, vm);
  mcsh_value* module_mcsh_value = mcsh_value_new_module(vm, module_mcsh);
  /* strmap_add(&parameters, "mcsh.argc", mcsh_value_construct_int(argc)); */
  /* strmap_add(&parameters, "mcsh.argv", */
  /*            mcsh_value_construct_list_charppc(vm, argc, argv)); */
  strmap_add(&module_mcsh_value->module->vars,
             "argc", mcsh_value_new_int(cmd->argc));
  strmap_add(&module_mcsh_value->module->vars,
             "argv" ,
             mcsh_value_new_list_charppc(vm, cmd->argc,
                                               cmd->argv));
  strmap_add(&parameters, "mcsh", module_mcsh_value);

  /* strmap_add(&parameters, "mcsh.argc", */
  /*            mcsh_value_construct_int(cmd->argc)); */
  /* strmap_add(&parameters, "mcsh.argv", */
  /*            mcsh_value_construct_list_charppc(vm, cmd->argc, */
  /*                                                  cmd->argv)); */

  vm_init_path(vm);
  mcsh_assign_specials(vm, &parameters);
  strmap_finalize(&parameters);
  add_globals(vm, &cmd->globals);
}

static bool
vm_init_path(mcsh_vm* vm)
{
  mcsh_log(&vm->logger, MCSH_LOG_CORE, MCSH_INFO,
           "vm_init_path: argc=%i", vm->argc);
  if (vm->argc == 0) return true;
  char* filename = vm->argv[0];
  struct stat s;
  int rc = stat(filename, &s);
  if (rc == -1)
  {
    mcsh_log(&vm->logger, MCSH_LOG_CORE, MCSH_INFO,
             "vm_init_path(): user script does not exist: '%s'",
             filename);
    return false;
  }

  char p[PATH_MAX];
  strcpy(p, filename);
  parent(p);
  mcsh_log(&vm->logger, MCSH_LOG_CORE, MCSH_INFO,
           "adding to path: '%s'", p);
  list_array_add(&vm->path, strdup(p));

  return true;
}

static void
add_globals(mcsh_vm* vm, strmap* map)
{
  mcsh_logger* logger = &vm->logger;
  for (size_t i = 0; i < map->size; i++)
  {
    char* key = map->keys[i];
    char* data = map->data[i];
    mcsh_value* value = mcsh_value_new_string(vm, data);
    LOG(MCSH_LOG_DATA, MCSH_DEBUG,
        "add_global: '%s'='%s'\n", key, data);
    table_add(&vm->globals, key, value);
  }
}

static void
parse_state_init(mcsh_parse_state* parse_state)
{
  parse_state->target = NULL;
  parse_state->id     = 0;
  parse_state->status = MCSH_PARSE_NONE;
  parse_state->output = NULL;
}

static void mcsh_stmts_init(mcsh_stmts* stmts);

void
mcsh_module_init(mcsh_module* module, mcsh_vm* vm)
{
  module->vm = vm;
  strcpy(module->source, "proto");
  strcpy(module->name,   "proto");
  module->instruction = 0;
  mcsh_stmts_init(&module->stmts);
  strmap_init(&module->vars, 4);
}

void
mcsh_stack_init(mcsh_stack* stack, mcsh_vm* vm)
{
  stack->vm = vm;
  stack->current = NULL;
}

void mcsh_entry_init_module(mcsh_entry* entry,
                            mcsh_module* module,
                            mcsh_entry* parent);

static mcsh_entry*
mcsh_entry_construct_module(mcsh_module* module,
                            mcsh_entry* parent)
{
  mcsh_entry* entry = malloc(sizeof(*entry));
  mcsh_entry_init_module(entry, module, parent);
  return entry;
}

static inline void mcsh_entry_init(mcsh_entry* entry,
                                   mcsh_entry* parent);

void
mcsh_entry_init_module(mcsh_entry* entry,
                       mcsh_module* module,
                       mcsh_entry* parent)
{
  mcsh_entry_init(entry, parent);
  entry->type = MCSH_ENTRY_MODULE;
  entry->stack  = &module->vm->stack;
  entry->depth  = 1;
  entry->args   = NULL;  // NYI - program argv
  entry->shift  = 0;
  entry->module = module;
}

static void
mcsh_entry_init_frame(mcsh_entry* entry, mcsh_entry* parent)
{
  mcsh_entry_init(entry, parent);
  entry->type   = MCSH_ENTRY_FRAME;
  entry->stack  = parent->stack;
  entry->depth  = parent->depth+1;
  entry->args   = NULL;
  entry->shift  = 0;
  entry->module = parent->module;
}

static void
mcsh_entry_init_scope(mcsh_entry* entry, mcsh_entry* parent)
{
  mcsh_entry_init(entry, parent);
  entry->type   = MCSH_ENTRY_SCOPE;
  entry->stack  = parent->stack;
  entry->depth  = parent->depth+1;
  entry->args   = parent->args;
  entry->shift  = parent->shift;
  entry->module = parent->module;
}

static void
mcsh_entry_init_macro(mcsh_entry* entry, mcsh_entry* parent)
{
  mcsh_entry_init(entry, parent);
  entry->type   = MCSH_ENTRY_MACRO;
  entry->stack  = parent->stack;
  entry->depth  = parent->depth+1;
  entry->args   = parent->args;
  entry->shift  = parent->shift;
  entry->module = parent->module;
}

static inline void
mcsh_entry_init(mcsh_entry* entry, mcsh_entry* parent)
{
  entry->id = counter++;
  // If parent is NULL, this is the main module
  entry->parent = parent;
  strmap_init(&entry->vars, 4);
}

void
mcsh_log_line(mcsh_module* module, const char* code)
{
  if (mcsh_log_check(&module->vm->logger, MCSH_LOG_PARSE, MCSH_WARN))
  {
    printf("--\n%s\n--\n", code);
    mcsh_module_print(module, 0);
  }
}

/**
   foundname: output buffer for located file in vm.path.
   Should be allocated by user to PATH_MAX
   @return True if found else false.
*/
static bool resolve_import(mcsh_vm* vm, const char* name,
                           bool add_suffix, char* foundname);

bool
mcsh_import(mcsh_module* caller, const char* name,
            mcsh_value** output, mcsh_status* status)
{
  mcsh_vm* vm = caller->vm;
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO,
           "attempt import: '%s'", name);

  char foundname[PATH_MAX];
  bool found = resolve_import(vm, name, true, foundname);
  if (! found)
  {
    mcsh_raise(status, NULL, 0, "mcsh.import_failed",
               "import error for '%s'", name);
    return true;
  }
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO,
           "import resolved: '%s'", foundname);
  FILE* fp = fopen(foundname, "r");
  if (fp == NULL)
  {
    char* msg = strerror(errno);
    mcsh_raise(status, NULL, 0, "mcsh.import_failed",
               "import error for '%s': %s", name, msg);
    return true;
  }
  char* code = slurp_fp(fp);
  mcsh_module* module = malloc_checked(sizeof(*module));
  mcsh_module_init(module, vm);
  mcsh_module_parse(foundname, name, code, module);
  mcsh_entry* entry = mcsh_entry_construct_module(module,
                                                  vm->stack.current);
  vm->stack.current = entry;
  mcsh_module_execute(module, output, status);
  // TODO: release entry->vars
  vm->stack.current = entry->parent;

  mcsh_value* value = mcsh_value_new_module(vm, module);
  strmap_add(&vm->stack.current->vars, name, value);
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO,
           "added: '%s'", name);
  value->refs++;

  status->code = MCSH_OK;
  return true;
}

bool mcsh_source_parse(const char* source,
                       char* code,
                       mcsh_module* module,
                       mcsh_stmts* stmts);

bool
mcsh_source(mcsh_module* caller, const char* name,
            mcsh_value** output, mcsh_status* status)
{
  mcsh_vm* vm = caller->vm;
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO,
           "attempt source: '%s'", name);

  char foundname[PATH_MAX];
  bool found = resolve_import(vm, name, false, foundname);
  if (! found)
  {
    mcsh_raise(status, NULL, 0, "mcsh.source_failed",
               "source error for '%s'", name);
    return true;
  }
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO,
           "resolved source: '%s'", foundname);
  FILE* fp = fopen(foundname, "r");
  if (fp == NULL)
    RAISE(status, NULL, 0, "mcsh.source_failed",
          "source error for '%s': %s", name, strerror(errno));

  char* code = slurp_fp(fp);

  mcsh_stmts stmts;
  mcsh_stmts_init(&stmts);

  mcsh_source_parse(foundname, code, caller, &stmts);

  mcsh_stmts_execute(caller, &stmts, output, status);

  status->code = MCSH_OK;
  return true;
}

bool
mcsh_eval(mcsh_module* caller, char* code,
          mcsh_value** output, mcsh_status* status)
{
  mcsh_vm* vm = caller->vm;
  mcsh_log(&vm->logger, MCSH_LOG_MODULE, MCSH_INFO, "eval ...");

  mcsh_stmts stmts;
  mcsh_stmts_init(&stmts);

  mcsh_source_parse("(eval)", code, caller, &stmts);

  mcsh_stmts_execute(caller, &stmts, output, status);

  status->code = MCSH_OK;
  return true;
}

static bool
resolve_import(mcsh_vm* vm, const char* name,
               bool add_suffix, char* foundname)
{
  size_t path_max = (size_t) PATH_MAX;
  struct stat s;
  for (int i = vm->path.size - 1; i >= 0; i--)
  {
    char* p = &foundname[0];
    append(p, vm->path.data[i], path_max);
    append(p, "/",              path_max);
    append(p, name,             path_max);
    if (add_suffix) append(p, ".mc", path_max);
    printf("try: %s\n", foundname);
    int rc = stat(foundname, &s);
    if (rc == 0)
    {
      printf("found.\n");
      return true;
    }
  }
  return false;
}

void mcsh_stmts_finalize(mcsh_module* module, mcsh_stmts* stmts);

void
mcsh_module_finalize(mcsh_module* module)
{
  mcsh_log(&module->vm->logger, MCSH_LOG_DATA, MCSH_INFO,
           "module_finalize: %s", module->name);
  strmap_finalize(&module->vars);
  mcsh_stmts_finalize(module, &module->stmts);
}

static void
mcsh_stmts_init(mcsh_stmts* stmts)
{
  list_array_init(&stmts->stmts, 8);
}

void mcsh_stmt_free(mcsh_module* module, mcsh_stmt* stmt);

void
mcsh_stmts_finalize(mcsh_module* module, mcsh_stmts* stmts)
{
  // printf("stmts_finalize: %zi\n", stmts->stmts.size);
  for (size_t i = 0; i < stmts->stmts.size; i++)
    mcsh_stmt_free(module, stmts->stmts.data[i]);
  list_array_finalize(&stmts->stmts);
}

void mcsh_thing_free(mcsh_thing* thing);

static void value_free_list(mcsh_logger* logger, mcsh_value* value);

static inline void
value_free(mcsh_logger* logger, mcsh_value* value)
{
  char name[64];

  switch (value->type)
  {
    case MCSH_VALUE_STRING:
    {
      mcsh_log(logger, MCSH_LOG_MEM, MCSH_INFO,
               "value_free: \"%s\"", value->string);
      free(value->string);
      break;
    }
    case MCSH_VALUE_LIST:
    {
      mcsh_log(logger, MCSH_LOG_MEM, MCSH_INFO,
               "value_free: %p list (%zi)",
               value, value->list->size);
      value_free_list(logger, value);
      break;
    }
    default:
    {
      mcsh_value_type_name(value->type, name);
      mcsh_log(logger, MCSH_LOG_MEM, MCSH_INFO,
               "value_free: skip: %p %s", value, name);
    }
  }
  free(value);
}

void
mcsh_value_free(mcsh_logger* logger, mcsh_value* value)
{
  value_free(logger, value);
}

void
mcsh_value_free_void(mcsh_logger* logger, void* p)
{
  mcsh_value* v = (mcsh_value*) p;
  value_free(logger, v);
}

static void
value_free_list(mcsh_logger* logger, mcsh_value* value)
{
  list_array* L = value->list;
  LOG(MCSH_LOG_MEM, MCSH_INFO, "free_list: %p", L);
  for (size_t i = 0; i < L->size; i++)
  {
    void* p = L->data[i];
    LOG(MCSH_LOG_MEM, MCSH_INFO, "drop_list: %zi: %p", i, p);
    mcsh_value_drop(logger, p);
  }
  list_array_free(L);
}

void
mcsh_stmt_free(mcsh_module* module, mcsh_stmt* stmt)
{
  mcsh_log(&module->vm->logger, MCSH_LOG_MEM, MCSH_INFO,
           "stmt_free(): %zi", stmt->things.size);
  list_array* L = &stmt->things;
  for (size_t i = 0; i < L->size; i++)
    mcsh_thing_free(L->data[i]);
  list_array_finalize(L);
  free(stmt);
}

void
stmts_free(mcsh_module* module, mcsh_stmts* stmts)
{
  for (size_t i = 0; i < stmts->stmts.size; i++)
    mcsh_stmt_free(module, stmts->stmts.data[i]);
}

static void
thing_subfun_free(mcsh_module* module, mcsh_subfun* subfun)
{
  stmts_free(module, &subfun->stmts);
  free(subfun);
}

void
mcsh_thing_free(mcsh_thing* thing)
{
  char string[64];
  mcsh_log(&thing->module->vm->logger, MCSH_LOG_MEM, MCSH_INFO,
           "thing_free(%p)");
  switch (thing->type)
  {
    case MCSH_THING_TOKEN:
    {
      free(thing->data.token->text);
      free(thing->data.token);
      break;
    }
    case MCSH_THING_SUBFUN:
    {
      thing_subfun_free(thing->module, thing->data.subfun);
      break;
    }
    default:
    {
      thing_str(thing, string);
      mcsh_log(&thing->module->vm->logger, MCSH_LOG_MEM, MCSH_INFO,
               "thing_free(): must free: %s", string);
    }
  }
  free(thing);
}

static inline void
mcsh_stmt_init(mcsh_stmt* stmt,
               mcsh_module* module, mcsh_thing* parent,
               int line)
{
  list_array_init(&stmt->things, 2);
  stmt->module = module;
  stmt->parent = parent;
  stmt->line   = line;
}

/** Get a unique object id for each parsed item */
static int
parse_id(void)
{
  return ++mcsh.parse_state.id;
}

void mcsh_script_source_set(char* text);
bool mcsh_script_preprocess(char* code);
void mcsh_script_clean(void);

// In mcsh-expr.l
void mcsh_expr_source_set(char* text);

void mcsh_node_print(mcsh_node* node, int indent);
static void mcsh_node_to_module(mcsh_module* module,
                                mcsh_thing* parent,
                                mcsh_node* node);

/**
   source: The filename or (stdin)
   name:   The module name
   code:   The code
   module: The module into which to put the AST
*/
bool
mcsh_module_parse(const char* source,
                  const char* name,
                  char* code,
                  mcsh_module* module)
{
  mcsh_log(&module->vm->logger, MCSH_LOG_PARSE, MCSH_DEBUG,
           "module_parse: %s", source);
  strcpy(mcsh.parse_state.source, source);
  strcpy(module->source,          source);
  strcpy(mcsh.parse_state.name,   name);
  strcpy(module->name,            name);
  mcsh.parse_state.status = MCSH_PARSE_START;
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));

  thing->type = MCSH_THING_MODULE;
  thing->module = module;
  thing->data.module = module;
  thing->module = module;
  mcsh.parse_state.target = thing;

  mcsh_script_preprocess(code);

  mcsh_script_source_set(code);

  // Call to bison parser:
  mcsh_script_parse();

  if (mcsh_log_check(&module->vm->logger, MCSH_LOG_PARSE, MCSH_TRACE))
    mcsh_node_print(mcsh.parse_state.output, 0);

  mcsh_node_to_module(module, NULL, mcsh.parse_state.output);

  mcsh_script_clean();

  if (mcsh.parse_state.status != MCSH_PARSE_OK)
  {
    printf("mcsh: parse failed!\n");
    return false;
  }
  return true;
}

static void node_to_stmts(mcsh_module* module, mcsh_thing* parent,
                          mcsh_node* node,
                          mcsh_stmts* stmts,
                          bool* added);

bool
mcsh_source_parse(const char* source,
                  char* code,
                  mcsh_module* module,
                  mcsh_stmts* stmts)
{
  mcsh_log(&module->vm->logger, MCSH_LOG_PARSE, MCSH_DEBUG,
           "module_parse: %s", source);
  strcpy(mcsh.parse_state.source, source);

  mcsh_script_preprocess(code);

  mcsh_script_source_set(code);

  // Call to bison parser:
  mcsh_script_parse();

  if (mcsh_log_check(&module->vm->logger, MCSH_LOG_PARSE, MCSH_FATAL))
    mcsh_node_print(mcsh.parse_state.output, 0);

  bool added = false;
  node_to_stmts(module, NULL, mcsh.parse_state.output, stmts, &added);

  mcsh_script_clean();

  if (mcsh.parse_state.status != MCSH_PARSE_OK)
  {
    printf("mcsh: source parse  failed!\n");
    return false;
  }
  return true;
}

static inline mcsh_thing*
mcsh_thing_construct_token(mcsh_module* module, const char* token)
{
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
  thing->type = MCSH_THING_TOKEN;
  thing->data.token = malloc_checked(sizeof(mcsh_token));
  thing->data.token->text = strdup(token);
  thing->module = module;
  return thing;
}

static inline mcsh_thing*
mcsh_thing_construct_block(mcsh_module* module, mcsh_thing* parent,
                           int line)
{
  mcsh_block* block = malloc_checked(sizeof(mcsh_block));
  block->id = parse_id();
  block->line = line;
  list_array_init(&block->stmts.stmts, 2);
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
  thing->type = MCSH_THING_BLOCK;
  thing->data.block = block;
  thing->parent = parent;
  thing->module = module;
  return thing;
}

static inline mcsh_thing*
mcsh_thing_construct_subcmd(mcsh_module* module, mcsh_thing* parent,
                            UNUSED int line)
{
  mcsh_subcmd* subcmd = malloc_checked(sizeof(mcsh_subcmd));
  /* subcmd->id = parse_id(); */
  /* subcmd->line = line; */
  list_array_init(&subcmd->stmts.stmts, 2);
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
  thing->type = MCSH_THING_SUBCMD;
  thing->data.subcmd = subcmd;
  thing->parent = parent;
  thing->module = module;
  return thing;
}

static inline mcsh_thing*
mcsh_thing_construct_subfun(mcsh_module* module, mcsh_thing* parent,
                            UNUSED int line)
{
  mcsh_subfun* subfun = malloc_checked(sizeof(mcsh_subfun));
  /* subfun->id = parse_id(); */
  /* subfun->line = line; */
  list_array_init(&subfun->stmts.stmts, 2);
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
  thing->type = MCSH_THING_SUBFUN;
  thing->data.subfun = subfun;
  thing->parent = parent;
  thing->module = module;
  return thing;
}

static inline mcsh_stmt*
mcsh_stmt_construct(mcsh_module* module, mcsh_thing* parent,
                    int line)
{
  mcsh_stmt* stmt = malloc_checked(sizeof(mcsh_stmt));
  mcsh_stmt_init(stmt, module, parent, line);
  return stmt;
}

// static bool mcsh_add_stmt(mcsh_stmt* stmt);

/* void */
/* mcsh_stmt_end2() */
/* { */
/*   char text[64]; */
/*   thing_str(mcsh.parse_state.target, text); */
/*   printf("stmt_end() in: %s\n", text); */
/*   if (mcsh.parse_state.target->type == MCSH_THING_STMT  || */
/*       mcsh.parse_state.target->type == MCSH_THING_BLOCK || */
/*       mcsh.parse_state.target->type == MCSH_THING_SUBFUN) */
/*   { */
/*     if (mcsh.parse_state.target->type == MCSH_THING_STMT) */
/*     { */
/*       mcsh_thing* tgt = mcsh.parse_state.target; */
/*       mcsh_stmt* stmt = tgt->data.stmt; */
/*       thing_str(mcsh.parse_state.target, text); */
/*       printf("parse target is stmt: %s\n", text); */
/*       mcsh.parse_state.target = tgt->parent; */
/*       printf("line: %i\n", __LINE__); */
/*       thing_str(mcsh.parse_state.target, text); */
/*       printf("parse target is now: %s\n", text); */
/*       mcsh_add_stmt(stmt); */
/*       free(tgt); // Free the old parse_target */
/*     } */
/*     else if (mcsh.parse_state.target->type == MCSH_THING_BLOCK) */
/*     { */
/*       // Empty stmt */
/*       /\* mcsh_thing* parent = mcsh.parse_state.target->parent; *\/ */
/*       /\* if (parent == NULL) *\/ */
/*       /\*   printf("parent is NULL\n"); *\/ */
/*       /\* mcsh.parse_state.target = parent; *\/ */
/*       printf("empty stmt in block\n"); */
/*     } */
/*     else */
/*       valgrind_fail(); */
/*   } */
/*   else if (mcsh.parse_state.target->type == MCSH_THING_MODULE) */
/*   { */
/*     printf("mcsh_stmt_end() MODULE ...\n"); */
/*   } */
/*   else valgrind_assert(false); */
/* } */

/*
static bool
mcsh_add_stmt(mcsh_stmt* stmt)
{
  bool result = false;
  if (mcsh.parse_state.target->type == MCSH_THING_MODULE)
  {
    mcsh_module* module = mcsh.parse_state.target->module;
    list_array_add(&module->stmts.stmts, stmt);
  }
  else if (mcsh.parse_state.target->type == MCSH_THING_BLOCK)
  {
    mcsh_block* block = mcsh.parse_state.target->data.block;
    list_array_add(&block->stmts.stmts, stmt);
  }
  else if (mcsh.parse_state.target->type == MCSH_THING_SUBFUN)
  {
    mcsh_subfun* subfun = mcsh.parse_state.target->data.subfun;
    list_array_add(&subfun->stmts.stmts, stmt);
  }
  else valgrind_fail();
  return result;
}
*/

void
mcsh_block_start()
{
  int line = mcsh_script_get_lineno();
  // Start a new block
  mcsh_thing* parent = mcsh.parse_state.target;
  mcsh_thing* thing =
    mcsh_thing_construct_block(parent->module, parent, line);
  char text[64];
  thing_str(thing, text);
  printf("block_start(): %s\n", text);
  mcsh.parse_state.target = thing;
}

void mcsh_block_end2()
{
  char text[64];
  mcsh_thing* this = mcsh.parse_state.target;
  thing_str(this, text);
  printf("block_end(): %s\n", text);
  // mcsh_block* this = tgt->data.block;

  mcsh_thing* parent = mcsh.parse_state.target->parent;
  thing_str(parent, text);
  printf("parent: %s\n", text);
  if (parent->type == MCSH_THING_STMT)
  {
    mcsh_stmt* stmt = parent->data.stmt;
    list_array_add(&stmt->things, this);
    /* mcsh_block* block = mcsh.parse_target->data.block; */
    /* list_array_add(&block->stmts.stmts, stmt); */
    mcsh.parse_state.target = parent;
  }
  else
  {
    printf("attempt to put a block at beginning of statement!\n");
    fflush(stdout);
    exit(1);
    // valgrind_fail();
  }
}

void
mcsh_subfun_start()
{
  mcsh_subfun* subfun = malloc_checked(sizeof(mcsh_subfun));
  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
  thing->type = MCSH_THING_SUBFUN;
  thing->data.subfun = subfun;
  list_array_add(&terms_in, thing);
}

void
mcsh_module_end()
{
  assert(mcsh.parse_state.target->type == MCSH_THING_MODULE);
  /* printf("mcsh_module_end() ... stmts=%zi\n", */
  /*        mcsh.parse_state.target->module->stmts.stmts.size); */
}

static void hp_directive(mcsh_vm* vm, buffer* line, const char** start);

void
mcsh_hp(mcsh_vm* vm, const char* text)
{
  buffer line;
  buffer_init(&line, 128);
  const char* p = text;
  bool done = false;
  while (!done)
  {
    switch (*p)
    {
      case '\n':
        puts(line.data);
        buffer_reset(&line);
        break;
      case '\0':
        done = true;
        break;
      case '<':
        hp_directive(vm, &line, &p);
        break;
      default:
        buffer_catc(&line, *p);
        break;
    }
    p++;
  }
}

static void hp_variable(mcsh_vm* vm, buffer* line,
                        const char** start);

static void
hp_directive(mcsh_vm* vm, buffer* line, const char** start)
{
  const char* p = *start;
  switch (*(p+1))
  {
    case '$':
      hp_variable(vm, line, start);
      break;
    default:
      buffer_catc(line, '<');
      break;
  }
}

static void
hp_variable(mcsh_vm* vm, buffer* line, const char** start)
{
  buffer code;
  buffer_init(&code, 128);
  const char* p = *start;
  p++;
  bool done = false;
  while (!done)
  {
    switch (*p)
    {
      case '>':
        *start = p;
        done = true;
        break;
      default:
        buffer_catc(&code, *p);
        break;
    }
    p++;
  }
  mcsh_value* result;
  mcsh_status status;
  mcsh_token_to_value(&vm->logger,
                      vm->stack.current,
                      code.data,
                      (void*) &result,
                      &status);
  char t[1024];
  mcsh_to_string(&vm->logger, t, 1024, result);
  mcsh_value_free(&vm->logger, result);
  buffer_cat(line, t);
}

mcsh_value*
mcsh_value_new_string(mcsh_vm* vm, const char* s)
{
  mcsh_value* value = malloc_checked(sizeof(mcsh_value));
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_TRACE,
           "new string: %p \"%s\"", value, s);
  mcsh_value_init_string(value, strdup(s));
  return value;
}

mcsh_value*
mcsh_value_new_string_null()
{
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_string(result, NULL);
  return result;
}

mcsh_value*
mcsh_value_new_null()
{
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_null(result);
  return result;
}

mcsh_value*
mcsh_value_new_link(mcsh_value* target)
{
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init(result);
  result->type = MCSH_VALUE_LINK;
  result->link = target;
  result->refs = 1;
  return result;
}

mcsh_value*
mcsh_value_new_int(int64_t i)
{
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_int(result, i);
  return result;
}

mcsh_value*
mcsh_value_new_float(double f)
{
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_float(result, f);
  return result;
}

mcsh_value*
mcsh_value_new_list_sized(mcsh_vm* vm, size_t size)
{
  mcsh_value* value = malloc(sizeof(mcsh_value));
  mcsh_value_init_list_sized(value, size);
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_DEBUG,
             "value new L: %p size=%zi", value, size);
  return value;
}

mcsh_value*
mcsh_value_new_list(mcsh_vm* vm)
{
  return mcsh_value_new_list_sized(vm, 4);
}

static inline void mcsh_list_add(mcsh_value* list, mcsh_value* value);

mcsh_value*
mcsh_value_new_list_charppc(mcsh_vm* vm, int argc, char** argv)
{
  mcsh_value* result =
    mcsh_value_new_list_sized(vm, argc);
  for (int i = 0; i < argc; i++)
  {
    mcsh_value* value = mcsh_value_new_string(vm, argv[i]);
    mcsh_list_add(result, value);
    mcsh_value_grab(&vm->logger, value);
  }
  return result;
}

mcsh_value*
mcsh_value_new_table(mcsh_vm* vm, size_t size)
{
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_DEBUG,
           "value new table: size=%zi", size);
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_table(result, size);
  return result;
}

mcsh_value*
mcsh_value_new_module(mcsh_vm* vm, mcsh_module* module)
{
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_DEBUG,
           "value new module");
  mcsh_value* result = malloc(sizeof(mcsh_value));
  mcsh_value_init_module(result, module);
  return result;
}

static mcsh_value*
mcsh_value_new_block(mcsh_block* block)
{
  mcsh_value* result = malloc_checked(sizeof(mcsh_value));
  result->type  = MCSH_VALUE_BLOCK;
  result->block = block;
  result->refs  = 0;
  return result;
}

mcsh_value*
mcsh_value_new_activation(mcsh_activation* activation)
{
  mcsh_value* result = malloc_checked(sizeof(mcsh_value));
  result->activation = activation;
  result->type = MCSH_VALUE_ACTIVATION;
  result->refs = 0;
  return result;
}

mcsh_value*
mcsh_value_clone(mcsh_value* value)
{
  mcsh_value* result = malloc_checked(sizeof(mcsh_value));
  result->type = value->type;
  result->string = strdup(value->string);
  return result;
}

void
mcsh_value_grab(mcsh_logger* logger, mcsh_value* value)
{
  valgrind_assert(value != NULL);
  value->refs++;
  mcsh_log(logger, MCSH_LOG_MEM, MCSH_INFO,
           "grab: %p %i", value, value->refs);
}

void
mcsh_value_drop(mcsh_logger* logger, mcsh_value* value)
{
  valgrind_assert_msg(value != NULL, "drop(): value == NULL!");
  valgrind_assert_msg(value->refs > 0,
                      "drop(): value %p refs == %i",
                      value, value->refs);
  value->refs--;
  mcsh_log(logger, MCSH_LOG_MEM, MCSH_INFO,
           "drop: %p %i", value, value->refs);
  if (value->refs == 0)
    mcsh_value_free(logger, value);
}

void
mcsh_value_assign(mcsh_value* target, mcsh_value* value)
{
  // TODO: Free old data
  switch (value->type)
  {
    case MCSH_VALUE_NULL:
      target->link = &mcsh_null;
      break;
    case MCSH_VALUE_STRING:
        target->string = strdup(value->string);
      break;
    case MCSH_VALUE_INT:
      target->type = MCSH_VALUE_INT;
      target->integer = value->integer;
      break;
    case MCSH_VALUE_FLOAT:
      target->number = value->number;
      break;
    case MCSH_VALUE_LIST:
      assert(false);
      break;
    case MCSH_VALUE_TABLE:
      assert(false);
      break;
    case MCSH_VALUE_BLOCK:
      assert(false);
      break;
    case MCSH_VALUE_FUNCTION:
      assert(false);
      break;
    case MCSH_VALUE_MODULE:
      assert(false);
      break;
    case MCSH_VALUE_LINK:
      assert(false);
      break;
    case MCSH_VALUE_ACTIVATION:
      assert(false);
      break;
    case MCSH_VALUE_ANY:
      // A real value cannot have type ANY
      assert(false);
      break;
  }
  target->type = value->type;
}

static inline void
mcsh_list_add(mcsh_value* list, mcsh_value* value)
{
  assert(list->type == MCSH_VALUE_LIST);
  list_array_add(list->list, value);
}

void
mcsh_table_add(mcsh_logger* logger,
               mcsh_value* table, mcsh_value* key, mcsh_value* value)
{
  assert(table->type == MCSH_VALUE_TABLE);
  const int max = 4096;
  char k[max];
  mcsh_to_string(logger, k, max, key);
  table_add(table->table, k, value);
}

static mcsh_function* mcsh_function_new(mcsh_module* module,
                                        mcsh_fn_type type,
                                        const char* name,
                                        mcsh_block* arglist,
                                        mcsh_block* code,
                                        mcsh_status* status);

mcsh_value*
mcsh_value_new_function(mcsh_module* module,
                        mcsh_fn_type type,
                        const char* name,
                        mcsh_block* arglist,
                        mcsh_block* code,
                        mcsh_status* status)
{
  mcsh_value* value = malloc_checked(sizeof(mcsh_value));
  mcsh_value_init(value);
  value->type       = MCSH_VALUE_FUNCTION;
  value->function   = mcsh_function_new(module,
                                        type, name, arglist,
                                        code,
                                        status);
  return value;
}

static inline void
mcsh_signature_init_block(mcsh_signature* sg,
                          mcsh_vm* vm,
                          mcsh_block* sgtokens,
                          mcsh_status* status);

static mcsh_function*
mcsh_function_new(mcsh_module* module,
                  mcsh_fn_type type,
                  const char* name, mcsh_block* sgtokens,
                  mcsh_block* code,
                  mcsh_status* status)
{
  mcsh_function* result = malloc_checked(sizeof(*result));
  result->type = type;
  result->name = strdup(name);

  /*
  mcsh_signature_init_block(&result->signature,
                            vm,
                            sgtokens,
                            status);
  */

  mcsh_signature_parse(module, &result->signature, sgtokens, status);
  mcsh_signature_print(&result->signature);


  result->block = code;
  return result;
}

static inline void
mcsh_signature_init_block(mcsh_signature* sg,
                          mcsh_vm* vm,
                          mcsh_block* sgtokens,
                          mcsh_status* status)
/** Initialize from function definition,
    where signature tokens are in a block { x y z }
    of mcsh_thing
*/
{
  sg->extras = false;

  mcsh_stmt* stmt0 = sgtokens->stmts.stmts.data[0];
  sg->count = stmt0->things.size;
  show("count: %u", sg->count);
  if (sg->count > 0)
  {
    mcsh_thing* thing = stmt0->things.data[sg->count-1];
    valgrind_assert(thing->type == MCSH_THING_TOKEN);
    char* tail = thing->data.token->text;
    if (strcmp(tail, "...") == 0)
    {
      sg->extras = true;
      sg->count--;
    }
  }

  sg->slots = malloc_checked(sg->count * sizeof(mcsh_slot));
  for (uint16_t i = 0; i < sg->count; i++)
  {
    mcsh_thing* thing = stmt0->things.data[i];
    mcsh_value* value;
    valgrind_assert(thing->type == MCSH_THING_TOKEN);
    printf("slot text: '%s'\n", thing->data.token->text);
    mcsh_token_to_value(&vm->logger,
                        vm->stack.current,
                        thing->data.token->text,
                        &value,
                        status);

    valgrind_assert(value->type == MCSH_VALUE_STRING);
    show("slot %u", i);
    sg->slots[i].name = strdup(value->string);
    sg->slots[i].dflt = NULL;
    if (sg->slots[i].dflt != NULL)
      mcsh_value_grab(NULL, sg->slots[i].dflt);
    show("loop");
  }
}

static inline void slot_init(mcsh_slot* slot,
                             char* name, mcsh_value* dflt,
                             mcsh_value_type type);

/** Used for unit test cases */
void
mcsh_signature_init(mcsh_signature* sg, uint16_t count,
                    char** names, mcsh_value** dflts,
                    bool extras)
{
  sg->count = count;
  sg->slots = malloc_checked(count * sizeof(mcsh_slot));
  for (uint16_t i = 0; i < count; i++)
  {
    mcsh_slot* slot = &sg->slots[i];
    slot_init(slot, names[i], dflts[i], MCSH_VALUE_ANY);
  }
  sg->extras = extras;
}

static inline void
slot_init(mcsh_slot* slot,
          char* name, mcsh_value* dflt, mcsh_value_type type)
{
  slot->name  = strdup_checked(name);
  slot->dflt  = dflt;
  slot->type  = type;
  if (slot->dflt != NULL)
    mcsh_value_grab(NULL, slot->dflt);
}

bool
mcsh_signature_parse(mcsh_module* module,
                     mcsh_signature* signature,
                     mcsh_block* sgtokens,
                     mcsh_status* status)
{
  mcsh_stmt* stmt0 = sgtokens->stmts.stmts.data[0];
  list_array* T = &stmt0->things;
  size_t N = list_array_size(T);

  signature->count = N;
  /// Are extra arguments allowed (...) ?
  signature->extras = false;
  signature->slots = malloc_checked(N * sizeof(mcsh_slot));
  for (size_t i = 0; i < N; i++)
  {
    mcsh_thing* thing = T->data[i];
    // mcsh_value* value;
    // mcsh_status status;

    // NTD => Name-Type-Dflt name[:type][=dflt]
    char* ntd = strdup(thing->data.token->text);
    mcsh_value* dflt = NULL;
    mcsh_value_type type = MCSH_VALUE_ANY;
    char* p;
    char* d = NULL; // default string
    char* q = NULL;
    p = strchr(ntd, '=');
    if (p != NULL)
    {
      d = p + 1;
      printf("dflt string: '%s'\n", d);
      *p = '\0';
      printf("dflt: '%s'   \n", d);
      bool rc = mcsh_token_to_value(&module->vm->logger,
                                    module->vm->stack.current, d,
                                    &dflt, status);
      CHECK0(rc);
    }
    p = strchr(ntd, ':');
    if (p != NULL)
    {
      q = p + 1;
      mcsh_value_type_code(q, &type);
      *p = '\0';
    }
    printf("name: '%s'   \n", ntd);
    // printf("name: '%s'  dflt: '%s'   type=%i \n", ntd, q, type);
    slot_init(&signature->slots[i],
              ntd, dflt, type);

    /*
    mcsh_token_to_value(&vm->logger,
                        vm->stack.current,
                        thing->data.token->text,
                        &value,
                        &status);

    valgrind_assert(value != NULL);
    valgrind_assert(value->type == MCSH_VALUE_STRING);
    */
  }

  printf("signature_parse: %zu\n", N);
  return true;
}

#define MAX_SIGNATURE 1024
void
mcsh_signature_print(mcsh_signature* sg)
{
  printf("signature (%u):\n", sg->count);
  char t[MAX_SIGNATURE];
  for (uint16_t i = 0; i < sg->count; i++)
  {
    printf("%s", sg->slots[i].name);
    if (sg->slots[i].dflt != NULL)
    {
      mcsh_to_string(NULL, t, MAX_SIGNATURE,
                     sg->slots[i].dflt);
      printf("=%s", t);
    }
    printf(" ");
  }
  if (sg->extras) printf("...");
  printf("\n");
}

void
mcsh_signature_finalize(mcsh_signature* sg)
{
  for (uint16_t i = 0; i < sg->count; i++)
  {
    free(sg->slots[i].name);
    if (sg->slots[i].dflt != NULL)
      mcsh_value_drop(NULL, sg->slots[i].dflt);
  }
  free(sg->slots);
}

bool mcsh_stmts_execute(mcsh_module* module, mcsh_stmts* stmts,
                        mcsh_value** output, mcsh_status* status);

bool
mcsh_module_execute(mcsh_module* module,
                    mcsh_value** output,
                    mcsh_status* status)
{
  // module->vm->logger.show_pid = true;
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
           "mcsh_module_execute() %i ...",
           module->instruction);
  bool rc = mcsh_stmts_execute(module, &module->stmts,
                               output, status);
  CHECK(rc, "module_execute: failed!");
  if (status->code == MCSH_EXCEPTION)
  {
    mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
             "mcsh_module_execute() %i: exception!",
             module->instruction);
  }
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
           "mcsh_module_execute() OK.");
  return true;
}

static bool mcsh_stmt_execute(mcsh_module* module, mcsh_stmt* stmt,
                              mcsh_value** output,
                              mcsh_status* status);

bool
mcsh_stmts_execute(mcsh_module* module, mcsh_stmts* stmts,
                   mcsh_value** output, mcsh_status* status)
{
  bool rc;
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
           "mcsh_stmts_execute() "
           "%p stmts=%zi output=%p @%i...",
           stmts,
           stmts->stmts.size, output, module->instruction);
  size_t i;
  for (i = module->instruction; i+1 < stmts->stmts.size; i++)
  {
    mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
               "execute: stmt: %zi: ...", i);
    // Need tmp- non-last statements (macros) may MCSH_RETURN
    mcsh_value* tmp = NULL;
    rc = mcsh_stmt_execute(module, stmts->stmts.data[i],
                           &tmp, status);
    CHECK(rc, "stmts_execute: stmt failed1: %zi\n", i);
    switch (status->code)
    {
      case MCSH_BREAK:
      case MCSH_CONTINUE:
      {
        return true;
      }
      case MCSH_RETURN:
      {
        mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
                 "execute: caught RETURN");
        *output = tmp;
        return true;
      }
      case MCSH_EXIT:
      {
        return true;
      }
      case MCSH_EXCEPTION:
      {
        return true;
      }
      default: ; // continue to next statement
    }
  }
  if (i < stmts->stmts.size)
  {
    mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_TRACE,
             "execute: stmt-last: %zi: ...", i);
    rc = mcsh_stmt_execute(module, stmts->stmts.data[i],
                           output, status);
    // valgrind_assert_msg(rc, "stmt failed2");
    CHECK(rc, "stmts_execute last: stmt failed2:  %zi", i);
    switch (status->code)
    {
      case MCSH_BREAK:
      case MCSH_CONTINUE:
      case MCSH_RETURN:
        return true;
      case MCSH_EXIT:
        // printf("execute: stmt exited!\n");
        return true;
      case MCSH_EXCEPTION:
        printf("execute: exception!\n");
        return true;
      default:
        ;
    }

    // printf("execute: output %p %p\n", output, *output);
    if (output == NULL)
    {
      ;
      printf("execute: stmt-out: output==NULL\n");
      // return false;
    }
    else if (*output == NULL)  // Statement did not return a value - OK
    {

      fail("execute: stmt-out: *output==NULL\n");
      // return false;
    }
    else if (*output != NULL)
    {
      // printf("output: %p\n", *output);
      // printf("output type: %i\n", (*output)->type);
      if ((*output)->type == MCSH_VALUE_STRING)
      {
        // printf("execute: stmt-out: '%s'\n", (*output)->string);
      }
      else
      {
        // printf("execute: stmt-out: non-string\n");
      }
    }
  }
  // printf("mcsh_stmts_execute() OK.\n");
  return true;
}

static bool add_word_split(list_array* args, mcsh_value* value);

static bool do_token(mcsh_logger* logger, mcsh_module* module,
                     mcsh_thing* token, list_array* values,
                     mcsh_status* status);

static inline bool is_keyword(const char* t);

static bool do_keyword(mcsh_logger* logger, mcsh_module* module,
                       const char* command,
                       list_array* values, mcsh_value** output,
                       mcsh_status* status);

static bool mcsh_value_call(mcsh_module* module,
                            mcsh_value* f, list_array* A,
                            mcsh_value** output, mcsh_status* status);

static bool
mcsh_stmt_execute(mcsh_module* module, mcsh_stmt* stmt,
                  mcsh_value** output, mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;
  status->code = MCSH_OK;  // default
  if (stmt->things.size == 0)
  {
    printf("stmt_execute(): empty\n");
    maybe_assign(output, &mcsh_null);
    return true;
  }
  mcsh_thing* thing = stmt->things.data[0];
  if (thing->type == MCSH_THING_BLOCK)
  {
    valgrind_fail_msg("received block as command!\n");
    maybe_assign(output, &mcsh_null);
    return true;
  }

  // Array of mcsh_value*
  list_array values;
  list_array_init(&values, stmt->things.size);
  for (size_t i = 0; i < stmt->things.size; i++)
  {
    mcsh_thing* token = stmt->things.data[i];
    // printf("token type: %i\n", token->type);
    do_token(logger, module, token, &values, status);
    if (status->code == MCSH_EXCEPTION)
      return true;
  }

  mcsh_value* command_value = values.data[0];
  if (command_value->type != MCSH_VALUE_STRING)
  {
    char t[1024];
    mcsh_to_string(logger, t, 1024, command_value);
    RAISE(status, NULL, 0, "mcsh.invalid_command",
          "command not a string: '%s'", t);
  }

  mcsh_value* f; // used if function
  bool rc;

  char* command = command_value->string;
  mcsh_log(logger, MCSH_LOG_CONTROL, MCSH_DEBUG,
           "command string: %p '%s'", command, command);

  mcsh_log(logger, MCSH_LOG_CONTROL, MCSH_DEBUG,
           "command args: %zi", values.size);

  if (is_keyword(command))
  {
    do_keyword(logger, module, command, &values, output, status);
  }
  else if (mcsh_stack_search(module->vm->stack.current, command, &f))
    // table_search(&module->vm->globals, command, (void*) &f)
  {
    char t[64];
    mcsh_to_string(logger, t, 64, f);
    // printf("found: %p '%s'\n", f, t);
    rc = mcsh_value_call(module, f, &values, output, status);
    CHECK(rc, "value_call failed.");
  }
  else if (mcsh_builtins_has(command))
  {
    LOG(MCSH_LOG_CONTROL, MCSH_WARN, "builtin execute: %s", command);
    mcsh_builtins_execute(module, &values, output, status);
    LOG(MCSH_LOG_CONTROL, MCSH_WARN, "builtin done: %s", command);
  }
  else
  {
    LOG(MCSH_LOG_CONTROL, MCSH_INFO,
        "unknown command: '%s'", command);
    mcsh_raise(status, NULL, 0, "mcsh.exception.unknown_command",
               "unknown command: '%s' in %s %s:%i",
               command, stmt->module->name,
               stmt->module->source, stmt->line);
  }

  // Need to start ref counting.
  // This fails when passing value for builtin 'set'
  // list_array_demolish_callback(&values, mcsh_value_free_void);
  return true;
}

static bool mcsh_do_if(mcsh_module* module, list_array* args,
                       mcsh_value** output, mcsh_status* status);
static bool mcsh_do_loop(mcsh_module* module, list_array* args,
                         mcsh_value** output, mcsh_status* status);
static bool mcsh_do_foreach(mcsh_module* module, list_array* args,
                            mcsh_value** output, mcsh_status* status);
static bool mcsh_do_for(mcsh_module* module, list_array* args,
                        mcsh_value** output, mcsh_status* status);
static bool mcsh_do_repeat(mcsh_module* module, list_array* args,
                           mcsh_value** output, mcsh_status* status);

static void build_keywords(char* keyword_string);

#define KEYWORD_TOTAL 1024

static char* keyword_list[] =
  {
    "if",
    "do",
    "loop",
    "for",
    "foreach",
    "repeat",
    "return",
    NULL
  };

static inline bool
is_keyword(const char* command)
{
  static bool built_keywords = false;
  static char keyword_string[KEYWORD_TOTAL];
  char t[128];
  t[0] = ' ';
  strcpy(&t[1], command);
  strcat(t, " ");
  if (!built_keywords)
  {
    build_keywords(&keyword_string[0]);
    built_keywords = true;
    // printf("keyword_string: '%s'\n", keyword_string);
  }
  bool result = (strstr(keyword_string, t) != NULL);
  return result;
}

static void
build_keywords(char* keyword_string)
{
  char* p = keyword_string;
  int i = 0;
  size_t space = KEYWORD_TOTAL;
  *p = ' ';
  p++;
  char* q;
  while (true)
  {
    q = p;
    if (keyword_list[i] == NULL) break;
    append(p, keyword_list[i], space);
    // t = strlcpy(p, keyword_list[i], space);
    // printf("t=%zu\n", t);
    space -= (p - q);
    append(p, " ", space);
    space -= 1;
    i++;
  }
  append(p, " ", space);
}

static bool
do_keyword(mcsh_logger* logger, mcsh_module* module,
           const char* command,
           list_array* values, mcsh_value** output,
           mcsh_status* status)
{
  bool rc;
  if (strcmp(command, "if") == 0)
  {
    mcsh_do_if(module, values, output, status);
  }
  else if (strcmp(command, "loop") == 0)
  {
    rc = mcsh_do_loop(module, values, output, status);
    CHECK(rc, "execute(): do_loop failed!");
    LOG(MCSH_LOG_CONTROL, MCSH_DEBUG,
        "do_loop returned: *output=%p\n", *output);
  }
  else if (strcmp(command, "foreach") == 0)
  {
    mcsh_do_foreach(module, values, output, status);
    printf("do_foreach returned: *output=%p\n", *output);
  }
  else if (strcmp(command, "for") == 0)
  {
    mcsh_do_for(module, values, output, status);
    printf("do_for returned: *output=%p\n", *output);
  }
  else if (strcmp(command, "repeat") == 0)
  {
    mcsh_do_repeat(module, values, output, status);
    printf("do_repeat returned: *output=%p\n", *output);
  }
  else if (strcmp(command, "return") == 0)
  {
    CHECK(values->size == 2, "return must have 1 argument!");
    *output = values->data[1];
    status->code = MCSH_RETURN;
  }
  return true;
}

static void bad_token(mcsh_logger* logger, mcsh_thing* token);

static bool
do_token(mcsh_logger* logger, mcsh_module* module,
         mcsh_thing* token, list_array* values,
         mcsh_status* status)
{
  bool rc;
  mcsh_value* value;
  mcsh_stmts* stmts;
  switch (token->type)
  {
    case MCSH_THING_TOKEN:
      // printf("convert: '%s'\n", token->data.token->text);
      rc = mcsh_token_to_value(logger,
                               module->vm->stack.current,
                               token->data.token->text,
                               (void*) &value,
                               status);
      CHECK(rc, "could not convert token to string: '%s'",
            token->data.token->text);
      // printf("TOKEN: '%s'\n", token->data.token->text);
      if (value->word_split)
        add_word_split(values, value);
      else
        list_array_add(values, value);
      break;
    case MCSH_THING_BLOCK:
      value = mcsh_value_new_block(token->data.block);
      list_array_add(values, value);
      break;
    case MCSH_THING_SUBCMD:
      LOG(MCSH_LOG_EVAL, MCSH_DEBUG,
               "subcmd execute...\n");
      stmts = &token->data.subcmd->stmts;
      mcsh_subcmd_capture(module, stmts, &value, status);
      // printf("value: %p\n", value);
      list_array_add(values, value);
      // printf("subcmd execute: '%s'\n", value->string);
      break;
    case MCSH_THING_SUBFUN:
      LOG(MCSH_LOG_EVAL, MCSH_INFO, "subfun execute...");
      stmts = &token->data.subfun->stmts;
      mcsh_stmts_execute(module, stmts, &value, status);
      if (status->code == MCSH_EXCEPTION)
      {
        LOG(MCSH_LOG_EVAL, MCSH_INFO, "subfun: exception!");
        return true;
      }
      LOG(MCSH_LOG_EVAL, MCSH_INFO, "subfun value: %p", value);
      list_array_add(values, value);
      if (value->type == MCSH_VALUE_STRING)
      {
        LOG(MCSH_LOG_EVAL, MCSH_INFO,
            "subfun string: '%s'", value->string);
      }
      else
      {
        LOG(MCSH_LOG_EVAL, MCSH_INFO,
            "subfun result: non-string");
      }
      LOG(MCSH_LOG_EVAL, MCSH_INFO, "subfun execute done.");
      break;
    default:
      bad_token(logger, token);
      break;
  }
  return rc;
}

static void
bad_token(mcsh_logger* logger, mcsh_thing* token)
{
  char thing_type[64];
  thing_str(token, thing_type);
  LOG(MCSH_LOG_CONTROL, MCSH_WARN,
      "bad token in stmt: type=%s\n", thing_type);
  valgrind_fail();
}

static bool add_word_split_list(list_array* args, mcsh_value* value);

static bool
add_word_split(list_array* args, mcsh_value* value)
{
  printf("split\n");

  switch (value->type)
  {
    case MCSH_VALUE_STRING:
      break;
    case MCSH_VALUE_LIST:
      add_word_split_list(args, value);
      break;
    default:
      list_array_add(args, value);
      break;
  }
  return true;
}

static bool
add_word_split_list(list_array* args, mcsh_value* value)
{
  for (size_t i = 0; i < value->list->size; i++)
  {
    list_array_add(args, value->list->data[i]);
    // printf("add\n");
  }
  return true;
}

void
raise_va(mcsh_status* status, char* source, int line,
         const char* tag, const char* fmt, va_list ap)
{
  status->code = MCSH_EXCEPTION;
  status->exception = malloc_checked(sizeof(mcsh_exception));
  status->exception->source = source;
  status->exception->line   = line;
  status->exception->tag    = strdup(tag);
  status->exception->cause  = NULL;
  size_t n = vasprintf(&status->exception->text, fmt, ap);
  va_end(ap);
  valgrind_assert(n > 0);
}

void
mcsh_raise(mcsh_status* status, char* source, int line,
           const char* tag, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  raise_va(status, source, line, tag, fmt, ap);
  va_end(ap);
}

void
mcsh_raise0(mcsh_status* status,
           const char* tag, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  raise_va(status, NULL, 0, tag, fmt, ap);
  va_end(ap);
}

void
mcsh_exception_free(mcsh_exception* exception)
{
  if (exception->cause != NULL)
    mcsh_exception_free(exception->cause);
  free(exception->tag);
  free(exception->text);
  free(exception);
}

void
mcsh_exception_reset(mcsh_status* status)
{
  mcsh_exception_free(status->exception);
  status->exception = NULL;
  status->code      = MCSH_OK;
}

bool
mcsh_stack_search(mcsh_entry* entry, const char* name,
                  mcsh_value** result)
{
  /* printf("stack_search(): %zi:%zi start:  '%s'\n", */
  /*        entry->depth, entry->id, name); */

  bool modules_only = false;
  char type_name[64];
  while (true)
  {
    if (modules_only)
      if (entry->type != MCSH_ENTRY_MODULE)
        goto loop;
    if (strmap_search(&entry->vars, name, (void**) result))
    {
      /* printf("stack_search(): %zi:%zi found:  '%s'\n", */
      /*        entry->depth, entry->id, name); */
      goto found;
    }
    if (entry->type == MCSH_ENTRY_MODULE)
    {
      if (strmap_search(&entry->module->vars, name, (void**) result))
      {
        printf("stack_search(): %zi:%zi found:  '%s'\n",
               entry->depth, entry->id, name);
        goto found;
      }
    }
    if (entry->type == MCSH_ENTRY_FRAME)
      // This is a stack frame (not a scope) -
      // now we can only search within modules
      modules_only = true;
    loop:
    if (entry->parent == NULL)
      break;
    entry = entry->parent;
  }
  // not found yet
  mcsh_vm* vm = entry->stack->vm;
  if (table_search(&vm->globals, name, (void**) result))
    goto found;

  if (mcsh_data_env(vm, name, result))
    goto found;

  if (mcsh_data_special(vm, name, result))
    goto found;

  // not found
  return false;

  found:
  // printf("stack variable is type: %s\n", );
  mcsh_value_type_name((*result)->type, type_name);
  // printf("stack variable is type: %s\n", type_name);
  return true;
}

bool
mcsh_do_if(mcsh_module* module, list_array* args,
           UNUSED mcsh_value** output, UNUSED mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;
  LOG(MCSH_LOG_CONTROL, MCSH_INFO,
      "mcsh_do_if(%zi) ...", args->size);

  unsigned int counter = 0;
  // Normally, we have a condition to check -
  //           on "or"/"else", we have no condition
  bool condition_or = false;
  while (true)
  {
    // printf("counter: %i\n", counter);
    mcsh_value* condition;
    int64_t condition_result;
    mcsh_value* keyword = args->data[counter++];
    char t[64];
    mcsh_to_string(logger, t, 64, keyword);
    // printf("type s: %s\n", t);

    // printf("type: %i\n", keyword->type);
    valgrind_assert(keyword->type == MCSH_VALUE_STRING);

    if (strcmp(keyword->string, "or") == 0)  // or else
    {
      condition_or = true;
      condition_result = 1;
    }
    else
    {
      valgrind_assert(strcmp(keyword->string, "if") == 0);
      condition = args->data[counter++];
      valgrind_assert_msg(condition->type == MCSH_VALUE_BLOCK,
                        "if condition must be a block!");
      // printf("actual condition. \n");
    }

    // printf("condition_or: %i\n", condition_or);

    mcsh_value* body = args->data[counter];
    valgrind_assert_msg(body->type == MCSH_VALUE_BLOCK,
                        "if body must be a block!");

    if (!condition_or)
    {
      mcsh_value* value_condition;
      mcsh_stmts_execute(module, &condition->block->stmts,
                         &value_condition, status);
      if (status->code == MCSH_EXCEPTION) return true;
      mcsh_value_integer(value_condition, &condition_result);
      LOG(MCSH_LOG_CONTROL, MCSH_INFO, "condition: %"PRId64,
          condition_result);
    }
    if (condition_result != 0)
    {
      LOG(MCSH_LOG_CONTROL, MCSH_INFO, "condition true");

      mcsh_value* value_body;
      mcsh_stmts_execute(module, &body->block->stmts,
                         &value_body, status);
      if (status->code == MCSH_EXCEPTION) return true;
      if (output != NULL) *output = value_body;
      break;
    }
    else
    {
      counter++;
      if (output != NULL)
        *output = &mcsh_null;
    }
    if (counter >= args->size)
    {
      LOG(MCSH_LOG_CONTROL, MCSH_INFO, "if exhausted.");
      break;
    }
  }
  return true;
}

static bool loop_while_until(mcsh_value* token, bool* positive,
                             mcsh_status* status);

static bool
mcsh_do_loop(mcsh_module* module, list_array* args,
              mcsh_value** output, UNUSED mcsh_status* status)
{
  unsigned int counter = 1;
  mcsh_value* value_condition;
  mcsh_value* body;
  mcsh_value* value_result = &mcsh_null;
  mcsh_value* condition_top = NULL;
  mcsh_value* condition_end = NULL;
  // "while" -> positive , "until" -> negative
  bool positive_top, positive_end = false;

  mcsh_logger* logger = &module->vm->logger;
  LOG(MCSH_LOG_CONTROL, MCSH_INFO, "mcsh_do_loop()");

  mcsh_value* v = args->data[counter];

  char t[64];
  int rc;

  if (v->type == MCSH_VALUE_STRING)
  {
    if (! loop_while_until(v, &positive_top, status)) return true;
    counter++;
    condition_top = args->data[counter];
    counter++;
  }

  body = args->data[counter];
  valgrind_assert(body->type == MCSH_VALUE_BLOCK);

  if (counter < args->size - 1)
  {
    counter++;
    v = args->data[counter];
    valgrind_assert(v->type == MCSH_VALUE_STRING);
    if (! loop_while_until(v, &positive_end, status)) return true;
    counter++;
    condition_end = args->data[counter];
    valgrind_assert(condition_end->type == MCSH_VALUE_BLOCK);
  }

  int64_t condition_result;

  while (true)
  {
    if (condition_top != NULL)
    {
      mcsh_stmts_execute(module, &condition_top->block->stmts,
                         &value_condition, status);
      mcsh_value_integer(value_condition, &condition_result);
      if (!positive_top) condition_result = ! condition_result;
      if (!condition_result) break;
    }

    LOG(MCSH_LOG_CONTROL, MCSH_INFO, "loop body...");
    mcsh_stmts_execute(module, &body->block->stmts,
                       &value_result, status);
    bool loop_break = false;
    switch (status->code)
    {
      case MCSH_PROTO:
      {
        valgrind_fail_msg("found MCSH_PROTO");
        break;
      }
      case MCSH_BREAK:
      {
        printf("loop: caught break:\n");
        status->code = MCSH_OK;
        printf("break *output=%p\n", *output);
        loop_break = true;
        break;
      }
      case MCSH_CONTINUE:
      {
        printf("loop: caught continue:\n");
        status->code = MCSH_OK;
        loop_break = true;
        break;
      }
      case MCSH_EXCEPTION:
      {
        printf("loop: exception...\n");
        printf("loop: %s\n", status->exception->tag);
        return true;
      }
      case MCSH_EXIT:
      case MCSH_RETURN:
      {
        mcsh_code_name(status->code, t);
        printf("loop: caught code: %s\n", t);
        return true;
      }
      case MCSH_OK:
        ;  // OK
    }
    if (loop_break) break;

    if (condition_end != NULL)
    {
      rc = mcsh_stmts_execute(module, &condition_end->block->stmts,
                              &value_condition, status);
      CHECK(rc, "do_loop(): stmt failed!");
      mcsh_value_integer(value_condition, &condition_result);
      if (!positive_end) condition_result = ! condition_result;
      if (!condition_result) break;
    }
  }
  LOG(MCSH_LOG_CONTROL, MCSH_INFO, "value_result: %p", value_result);
  maybe_assign(output, value_result);

  return true;
}

/**
   In TOKEN {condition_top} {body} TOKEN {condition_end}
   is TOKEN a "while" or "until",
   or an exception on error?
   @return True unless there is an exception
*/
static bool
loop_while_until(mcsh_value* token, bool* positive,
                 mcsh_status* status)
{
  if (strcmp(token->string, "while") == 0)
    *positive = true;
  else if (strcmp(token->string, "until") == 0)
    *positive = false;
  else
  {
    mcsh_raise(status, NULL, 0, "mcsh.exception.syntax",
               "bad loop keyword: '%s'", token->string);
    return false;
  }
  return true;
}

typedef struct
{
  bool loop_break  : 1;
  bool loop_return : 1;
} loop_result;

static inline loop_result loop_check(mcsh_status* status);

static bool
mcsh_do_foreach(mcsh_module* module, list_array* args,
                mcsh_value** output, mcsh_status* status)
{
  valgrind_assert_msg(args->size == 4, "foreach needs 4 arguments!");
  mcsh_value* name = args->data[1];
  mcsh_value* list = args->data[2];
  mcsh_value* body = args->data[3];
  mcsh_value* value_result;
  printf("foreach start...\n");
  for (unsigned int i = 0; i < list->list->size; i++)
  {
    printf("foreach iteration: %u\n", i);
    mcsh_value* item = list->list->data[i];
    mcsh_set_value(module, name->string, item, status);
    // TODO: check status
    mcsh_stmts_execute(module, &body->block->stmts,
                       &value_result, status);
    printf("executed\n");
    loop_result result = loop_check(status);
    if (result.loop_break)  break;
    if (result.loop_return) break;
  }
  maybe_assign(output, value_result);
  return true;
}

static bool
mcsh_do_for(mcsh_module* module, list_array* args,
            mcsh_value** output, mcsh_status* status)
{
  valgrind_assert_msg(args->size == 5, "for needs 5 arguments!");
  // for { init } { test } { post } { body }
  mcsh_value* init = args->data[1];
  mcsh_value* test = args->data[2];
  mcsh_value* post = args->data[3];
  mcsh_value* body = args->data[4];
  mcsh_value* value_post, * value_result;
  printf("for: start...\n");
  mcsh_stmts_execute(module, &init->block->stmts,
                     &value_result, status);

  while (true)
  {
    printf("for: test ...\n");
    mcsh_stmts_execute(module, &test->block->stmts,
                       &value_result, status);
    int64_t v;
    mcsh_value_integer(value_result, &v);
    if (v == 0) break;

    printf("for: iteration ...\n");
    mcsh_stmts_execute(module, &body->block->stmts,
                       &value_result, status);
    printf("executed\n");
    loop_result result = loop_check(status);
    if (result.loop_break) break;

    mcsh_stmts_execute(module, &post->block->stmts,
                       &value_post, status);
  }
  printf("for: done.\n");
  maybe_assign(output, value_result);
  return true;
}

static bool
mcsh_do_repeat(mcsh_module* module, list_array* args,
               mcsh_value** output, mcsh_status* status)
{
  int index = 1;
  mcsh_value* name;
  switch (args->size)
  {
    case 3:
      name = NULL;
      break;
    case 4:
      name = args->data[index++];
      break;
    default:
      valgrind_fail_msg("repeat needs 3 or 4 arguments!");
  }
  mcsh_value* stop = args->data[index++];
  mcsh_value* body = args->data[index++];

  int64_t s;
  mcsh_value_integer(stop, &s);

  mcsh_value* value_result;
  printf("repeat start...\n");
  for (unsigned int i = 0; i < s; i++)
  {
    printf("repeat iteration: %u\n", i);
    if (name != NULL)
    {
      mcsh_value* item = mcsh_value_new_int(i);
      mcsh_set_value(module, name->string, item, status);
      // TODO: check status
    }
    mcsh_stmts_execute(module, &body->block->stmts,
                       &value_result, status);
    printf("executed\n");
    loop_result result = loop_check(status);
    if (result.loop_break)  break;
    if (result.loop_return) break;
  }
  maybe_assign(output, value_result);
  return true;
}

static inline loop_result
loop_check(mcsh_status* status)
{
  char t[64];
  loop_result result = {0};
  switch (status->code)
  {
    case MCSH_PROTO:
      valgrind_fail_msg("found MCSH_PROTO");
      break;
    case MCSH_BREAK:
      printf("loop_check: caught break:\n");
      status->code = MCSH_OK;
      result.loop_break = true;
      break;
    case MCSH_CONTINUE:
      printf("loop: caught continue:\n");
      status->code = MCSH_OK;
      break;
    case MCSH_EXCEPTION:
      printf("loop: hit exception: '%s'\n", status->exception->tag);
      result.loop_return = true;
      break;
    case MCSH_EXIT:
    case MCSH_RETURN:
      mcsh_code_name(status->code, t);
      printf("loop: caught code: %s\n", t);
      result.loop_return = true;
      break;
    case MCSH_OK:
      ;  // OK
  }
  return result;
}

static bool set_params(list_array* A, mcsh_value* f,
                       mcsh_entry* entry, mcsh_status* status);

static bool
mcsh_value_call(mcsh_module* module, mcsh_value* f,
                list_array* A, mcsh_value** output,
                mcsh_status* status)
{
  if (f->type != MCSH_VALUE_FUNCTION)
    RAISE(status, NULL, 0, "mcsh.invalid_type",
          "attempt to call a variable that is not a function");
  mcsh_function* function = f->function;
  mcsh_block* block = function->block;
  // Default to success:
  status->code = MCSH_OK;
  mcsh_entry* entry = malloc(sizeof(*entry));
  switch (function->type)
  {
    case MCSH_FN_NORMAL:
      mcsh_entry_init_frame(entry, module->vm->stack.current);
      mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
               "call(): frame init: %zi:%zi",
               entry->depth, entry->id);
      break;
    case MCSH_FN_INPLACE:
      mcsh_entry_init_scope(entry, module->vm->stack.current);
      printf("call(): scope: %zi:%zi\n", entry->depth, entry->id);
      break;
    case MCSH_FN_MACRO:
      mcsh_entry_init_macro(entry, module->vm->stack.current);
      printf("call(): macro: %zi:%zi\n", entry->depth, entry->id);
      break;
  }
  module->vm->stack.current = entry;
  entry->args = A;
  bool rc;
  rc = set_params(A, f, module->vm->stack.current, status);
  CHECK(rc, "call(): set_params() failed!");
  PROPAGATE(status);
  rc = mcsh_stmts_execute(module, &block->stmts, output, status);
  CHECK(rc, "call(): failed for '%s'", function->name);
  mcsh_log(&module->vm->logger, MCSH_LOG_EVAL, MCSH_DEBUG,
           "call(): frame drop: %zi:%zi", entry->depth, entry->id);
  if (status->code == MCSH_RETURN)
    status->code = MCSH_OK;
  module->vm->stack.current = entry->parent;
  return true;
}

static inline void set_positional_next(mcsh_signature* sg,
                                       mcsh_value* value,
                                       mcsh_parameters* P);

static inline void set_positional_at(mcsh_signature* sg,
                                     mcsh_value* value,
                                     mcsh_parameters* P, uint16_t j);


static inline bool set_named(mcsh_signature* sg, mcsh_arg* arg,
                             mcsh_parameters* P);

static inline bool set_defaults(mcsh_signature* sg,
                                mcsh_parameters* P,
                                mcsh_status* status);

static inline void parameters_init(mcsh_parameters* P,
                                   mcsh_signature* sg);

bool
mcsh_parameterize(mcsh_signature* sg, list_array* A,
                  mcsh_parameters* P, mcsh_status* status)
/**
   A: list of mcsh_arg*
 */
{
  show("parameterize A=%zi...", A->size);
  parameters_init(P, sg);

  if (A->size > sg->count && ! sg->extras)
  {
    printf("too many arguments!\n");
    return false;
  }

  for (size_t i = 0; i < A->size; i++)
  {
    mcsh_arg* arg = list_array_get(A, i);
    if (arg->name == NULL)
      show("got: NULL");
    else
      show("got: '%s'", arg->name->string);
    if (arg->name == NULL)
      set_positional_next(sg, arg->value, P);
    else
      set_named(sg, arg, P);
  }

  set_defaults(sg, P, status);
  show("parameterize OK.");
  return true;
}

static inline void
parameters_init(mcsh_parameters* P, mcsh_signature* sg)
/** Initialize P using sg as a reference for sizes */
{
  P->count = 0;
  P->names  = calloc_checked(sg->count, sizeof(char*));
  P->values = calloc_checked(sg->count, sizeof(mcsh_value*));
  // Nothing is allocated if list_size == 0:
  int list_size = sg->extras ? 4 : 0;
  list_array_init(&P->extra_names,  list_size);
  list_array_init(&P->extra_values, list_size);
}

void
mcsh_parameters_finalize(mcsh_parameters* P)
{
  for (int i = 0; i < P->count; i++)
  {
    free(P->names[i]);
    mcsh_value_drop(NULL, P->values[i]);
  }
  free(P->names);
  free(P->values);
  list_array_finalize(&P->extra_names);
  list_array_finalize(&P->extra_values);
}

static inline void
set_positional_next(mcsh_signature* sg, mcsh_value* value,
                    mcsh_parameters* P)
{
  printf("set_positional:\n");
  for (uint16_t j = 0; j < sg->count; j++)
  {
    if (P->values[j] == NULL)
    {
      set_positional_at(sg, value, P, j);
      return;
    }
  }

  if (! sg->extras)
    valgrind_fail_msg("too many arguments!");
  printf("set extra:\n");
  list_array_add(&P->extra_names,  NULL);
  list_array_add(&P->extra_values, value);
}

static inline void
set_positional_at(mcsh_signature* sg, mcsh_value* value,
                  mcsh_parameters* P, uint16_t j)
{
  printf("%i orig %p\n", j, sg->slots[j].name);
  P->names[j] = strdup_null_checked(sg->slots[j].name);
  printf("%i name %p '%s'\n", j, P->names[j], P->names[j]);
  P->count++;
  P->values[j] = value;
  mcsh_value_grab(NULL, value);
}

static inline bool
set_named(mcsh_signature* sg, mcsh_arg* arg,
          mcsh_parameters* P)
{
  buffer name;
  buffer_init(&name, 64);
  mcsh_value_buffer(NULL, arg->name, &name);
  bool found = false;
  for (uint16_t j = 0; j < sg->count; j++)
  {
    if (strcmp(sg->slots[j].name, name.data) == 0)
    {
      if (P->values[j] != NULL)
        valgrind_fail();
      P->count++;
      P->names[j] = strdup_checked(sg->slots[j].name);
      P->values[j] = arg->value;
      mcsh_value_grab(NULL, arg->value);
      found = true;
      break;
    }
  }

  if (!found) valgrind_fail();

  buffer_finalize(&name);
  return true;
}

static inline bool
set_defaults(mcsh_signature* sg, mcsh_parameters* P,
             mcsh_status* status)
{
  for (uint16_t j = 0; j < sg->count; j++)
  {
    printf("check: j=%i\n", j);
    if (P->values[j] == NULL)
    {
      printf(" found NULL\n");
      if (sg->slots[j].dflt != NULL)
      {
        set_positional_at(sg, sg->slots[j].dflt, P, j);
      }
      else
      {
        printf("did not assign to: '%s'", sg->slots[j].name);
        RAISE(status, NULL, 0,
              "mcsh.invalid_arguments",
              "did not assign to: '%s'", sg->slots[j].name);
      }
    }
  }
  return true;
}

bool
mcsh_parameters_print(mcsh_parameters* P)
{
  buffer B;
  buffer_init(&B, 128);
  printf("parameters: [%p] %i+%zi\n",
         P, P->count, P->extra_names.size);
  for (uint16_t i = 0; i < P->count; i++)
  {
    mcsh_value_buffer(NULL, P->values[i], &B);
    printf(" %i '%s'='%s'\n", i, P->names[i], B.data);
    buffer_reset(&B);
  }
  if (P->extra_names.size > 0) printf("extras:\n");
  for (uint16_t i = 0; i < P->extra_names.size; i++)
  {
    char*       name  = list_array_get(&P->extra_names,  i);
    mcsh_value* value = list_array_get(&P->extra_values, i);
    mcsh_value_buffer(NULL, value, &B);
    printf(" %i '%s'='%s'\n", i, name, B.data);
    buffer_reset(&B);
  }

  buffer_finalize(&B);

  return true;
}

static bool
set_params(list_array* A, mcsh_value* f, mcsh_entry* entry,
           mcsh_status* status)
/** Set up mcsh_entry with function call parameterization
    A: list of mcsh_value* : including function name!
    f: mcsh_value<function>
    entry: the current stack frame
 */
{
  // mcsh_value* function_name_value = A->data[0];
  // char* function_name = function_name_value->string;

  mcsh_logger* logger = &entry->stack->vm->logger;

  LOG(MCSH_LOG_DATA, MCSH_INFO,
      "set_params: slots=%zi arguments=%zi",
      f->function->signature.count, A->size-1);

  mcsh_parameters P;

  // List of mcsh_arg:
  list_array L;
  list_array_init(&L, A->size - 1);
  // List A includes the function name: skip it:
  for (size_t j = 1; j < A->size; j++)
  {
    mcsh_arg* arg = malloc_checked(sizeof(*arg));
    arg->name = NULL;
    arg->value = list_array_get(A, j);
    list_array_add(&L, arg);
  }

  bool rc = mcsh_parameterize(&f->function->signature, &L, &P,
                              status);
  valgrind_assert(rc);
  mcsh_parameters_print(&P);

  int i = 0;
  for ( ; i < P.count; i++)
  {
    show("set parameter to stack entry: %i", i);
    char*       name  = P.names[i];
    LOG(MCSH_LOG_DATA, MCSH_TRACE, "param: '%s'", name);
    mcsh_value* value = P.values[i];
    strmap_add(&entry->vars, name, value);
    mcsh_value_grab(logger, value);
  }
  if (f->function->signature.extras)
  {
    size_t args_size = P.count - i;
    LOG(MCSH_LOG_DATA, MCSH_DEBUG, "args_size: %zi", args_size);
    mcsh_value* args =
      mcsh_value_new_list_sized(
        entry->stack->vm, args_size);
    for ( ; i < P.count; i++)
      list_array_add(args->list, A->data[i]);
    strmap_add(&entry->vars, "args", args);
  }
  return true;
}

static void print_spaces(int count);

void
mcsh_module_print(mcsh_module* module, int indent)
{
  print_spaces(indent);
  printf("<< module: [%i] \"%s\"\n",
         module->instruction, module->name);
  for (size_t i = 0; i < module->stmts.stmts.size; i++)
  {
    mcsh_stmt* stmt = module->stmts.stmts.data[i];
    mcsh_stmt_print(stmt, indent);
  }
  print_spaces(indent);
  printf(">>\n");
}

void
mcsh_stmt_print(mcsh_stmt* stmt, int indent)
{
  print_spaces(indent);
  printf("S:");
  for (size_t i = 0; i < stmt->things.size; i++)
  {
    mcsh_thing* thing = stmt->things.data[i];
    mcsh_thing_show(thing, indent);
    if (i < stmt->things.size - 1)
      printf(" ");
  }
  printf("\n");
}

void
mcsh_thing_show(mcsh_thing* thing, int indent)
{
  switch (thing->type)
  {
    case MCSH_THING_NULL:
      printf("SHOW:NULL!!!\n");
      break;
    case MCSH_THING_TOKEN:
      mcsh_token_print(thing->data.token);
      break;
    case MCSH_THING_STMT:
      mcsh_stmt_print(thing->data.stmt, indent);
      break;
    case MCSH_THING_BLOCK:
      mcsh_block_print(thing->data.block, indent);
      break;
    case MCSH_THING_SUBCMD:
      mcsh_subcmd_print(thing->data.subcmd, indent);
      break;
    case MCSH_THING_SUBFUN:
      mcsh_subfun_print(thing->data.subfun, indent);
      break;
    default:
      valgrind_fail();
  }
}

void
mcsh_block_print(mcsh_block* block, int indent)
{
  printf("{\n");
  for (size_t i = 0; i < block->stmts.stmts.size; i++)
  {
    mcsh_stmt* stmt = block->stmts.stmts.data[i];
    mcsh_stmt_print(stmt, indent+2);
  }
  print_spaces(indent);
  printf("}");
}

void
mcsh_subcmd_print(mcsh_subcmd* subcmd, int indent)
{
  printf("$((\n");
  for (size_t i = 0; i < subcmd->stmts.stmts.size; i++)
  {
    mcsh_stmt* stmt = subcmd->stmts.stmts.data[i];
    mcsh_stmt_print(stmt, indent+2);
  }
  print_spaces(indent);
  printf("))");
}

void
mcsh_subfun_print(mcsh_subfun* subfun, int indent)
{
  printf("((\n");
  for (size_t i = 0; i < subfun->stmts.stmts.size; i++)
  {
    mcsh_stmt* stmt = subfun->stmts.stmts.data[i];
    mcsh_stmt_print(stmt, indent+2);
  }
  print_spaces(indent);
  printf("))");
}

void
mcsh_token_print(mcsh_token* token)
{
  printf("TOKEN:'%s'", token->text);
}

static void
node_show_op(mcsh_node* node, int indent)
{
  mcsh_operator* op = node->children.data[0];
  char t[4];
  op_to_string(t, *op);
  // printf("OP: %i\n", *op);
  printf("OP: %s %p %i\n", t, node, node->type);
  int count = (*op == MCSH_OP_TERN) ? 3 : 2;
  for (int i = 1; i <= count; i++)
    mcsh_node_print(node->children.data[i], indent+2);
}

void
mcsh_node_print(mcsh_node* node, int indent)
{
  print_spaces(indent);
  print_spaces(indent);
  if (node == NULL)
  {
    printf("NULL\n");
    return;
  }

  switch (node->type)
  {
    case MCSH_NODE_TYPE_TOKEN:
      printf("TOKEN: '%s' line=%i\n",
             (char*) node->children.data[0], node->line);
      break;
    case MCSH_NODE_TYPE_OP:
      node_show_op(node, indent);
      break;
    case MCSH_NODE_TYPE_PAIR:
      printf("PAIR\n");
      mcsh_node_print(node->children.data[0], indent+2);
      mcsh_node_print(node->children.data[1], indent+2);
      break;
    case MCSH_NODE_TYPE_STMTS:
      printf("STMTS\n");
      mcsh_node_print(node->children.data[0], indent+2);
      mcsh_node_print(node->children.data[1], indent+2);
      break;
    case MCSH_NODE_TYPE_BLOCK:
      printf("BLOCK\n");
      mcsh_node_print(node->children.data[0], indent+2);
      break;
    case MCSH_NODE_TYPE_SUBCMD:
      printf("SUBCMD\n");
      mcsh_node_print(node->children.data[0], indent+2);
      break;
    case MCSH_NODE_TYPE_SUBFUN:
      printf("SUBFUN\n");
      mcsh_node_print(node->children.data[0], indent+2);
      break;
    default:
      valgrind_fail_msg("UNKNOWN: %i", node->type);
  }
}

static void
print_spaces(int count)
{
  for (int i = 0; i < count; i++)
    printf(" ");
}

static void
mcsh_node_to_module(mcsh_module* module, mcsh_thing* parent,
                    mcsh_node* node)
{
  valgrind_assert(node != NULL);
  bool added = false;
  node_to_stmts(module, parent, node, &module->stmts, &added);
}

static void pair_to_stmt(mcsh_module* module,
                         mcsh_thing* parent,
                         mcsh_node* node, mcsh_stmt* stmt);

static void
node_to_stmts(mcsh_module* module, mcsh_thing* parent,
              mcsh_node* node,
              mcsh_stmts* stmts,
              // Top-level call should initialize this to false:
              // if it wants a result (may be NULL)
              bool* added)
{
  // printf("node_to_stmts\n");
  valgrind_assert(node->type = MCSH_NODE_TYPE_STMTS);
  mcsh_node* left = node->children.data[0];
  if (left != NULL)
    node_to_stmts(module, parent, left, stmts, added);
  mcsh_node* right = node->children.data[1];
  if (right != NULL)
  {
    mcsh_stmt* stmt = mcsh_stmt_construct(module, NULL, right->line);
    list_array_add(&stmts->stmts, stmt);
    pair_to_stmt(module, parent, right, stmt);
    if (added != NULL && !*added)
    {
      module->instruction = stmts->stmts.size - 1;
      *added = true;
    }
  }
}

static mcsh_thing* node_to_thing(mcsh_module* module,
                                 mcsh_thing* parent,
                                 mcsh_node* node);

static void
pair_to_stmt(mcsh_module* module, mcsh_thing* parent,
             mcsh_node* node, mcsh_stmt* stmt)
{
  // printf("node_to_stmt\n");
  valgrind_assert(node->type = MCSH_NODE_TYPE_PAIR);
  mcsh_node* left = node->children.data[0];
  if (left != NULL)
    pair_to_stmt(module, parent, left, stmt);
  mcsh_node* right = node->children.data[1];
  valgrind_assert(right != NULL);
  mcsh_thing* thing = node_to_thing(module, parent, right);
  list_array_add(&stmt->things, thing);
}

mcsh_thing*
mcsh_thing_from_value(mcsh_module* module, mcsh_value* value)
{
  valgrind_assert(value->type == MCSH_VALUE_STRING);
  mcsh_thing* result =
    mcsh_thing_construct_token(module, value->string);
  return result;
}

static mcsh_thing* node_to_thing_token(mcsh_module* module,
                                       mcsh_node* node);
static mcsh_thing* node_to_thing_block(mcsh_module* module,
                                       mcsh_thing* parent,
                                       mcsh_node* node);
static mcsh_thing* node_to_thing_subcmd(mcsh_module* module,
                                       mcsh_thing* parent,
                                       mcsh_node* node);
static mcsh_thing* node_to_thing_subfun(mcsh_module* module,
                                       mcsh_thing* parent,
                                       mcsh_node* node);

static mcsh_thing*
node_to_thing(mcsh_module* module, mcsh_thing* parent,
              mcsh_node* node)
{
  mcsh_thing* thing = NULL;
  switch (node->type)
  {
    case MCSH_NODE_TYPE_TOKEN:
      thing = node_to_thing_token(module, node);
      break;
    case MCSH_NODE_TYPE_BLOCK:
      thing = node_to_thing_block(module, parent, node);
      break;
    case MCSH_NODE_TYPE_SUBCMD:
      thing = node_to_thing_subcmd(module, parent, node);
      break;
    case MCSH_NODE_TYPE_SUBFUN:
      thing = node_to_thing_subfun(module, parent, node);
      break;
    default:
      valgrind_fail();
  }
  return thing;
}

static mcsh_thing*
node_to_thing_token(mcsh_module* module, mcsh_node* node)
{
  char* token = node->children.data[0];
  // printf("node_to_thing_token: '%s'\n", token);
  mcsh_thing* thing = mcsh_thing_construct_token(module, token);
  return thing;
}

static mcsh_thing*
node_to_thing_block(mcsh_module* module, mcsh_thing* parent,
                    mcsh_node* node)
{
  mcsh_node* child = node->children.data[0];
  // printf("node_to_thing_block.\n");
  // mcsh_thing* thing = mcsh_thing_construct_token(token);
  mcsh_thing* block = mcsh_thing_construct_block(module, parent, -1);
  node_to_stmts(module, parent,
                child, &block->data.block->stmts, NULL);
  return block;
}

static mcsh_thing*
node_to_thing_subcmd(mcsh_module* module, mcsh_thing* parent,
                    mcsh_node* node)
{
  mcsh_node* child = node->children.data[0];
  mcsh_thing* subcmd = mcsh_thing_construct_subcmd(module, parent, -1);
  node_to_stmts(module, parent,
                child, &subcmd->data.subcmd->stmts, NULL);
  return subcmd;
}

static mcsh_thing*
node_to_thing_subfun(mcsh_module* module, mcsh_thing* parent,
                    mcsh_node* node)
{
  mcsh_node* child = node->children.data[0];
  // printf("node_to_thing_subfun.\n");
  // mcsh_thing* thing = mcsh_thing_construct_token(token);
  mcsh_thing* subfun = mcsh_thing_construct_subfun(module, parent, -1);
  node_to_stmts(module, parent,
                child, &subfun->data.subfun->stmts, NULL);
  return subfun;
}

static void
vm_global_free(UNUSED void* context,
               UNUSED const char* k, UNUSED void* v)
{
  // printf("vm_global_free: '%s'\n", k);
}

void
mcsh_entry_free(mcsh_entry* entry)
{
  mcsh_logger* logger = &entry->stack->vm->logger;
  LOG(MCSH_LOG_DATA, MCSH_DEBUG,
      "entry_free: %p %zi", entry, entry->vars.size);
  strmap* map = &entry->vars;
  for (size_t i = 0; i < map->size; i++)
    if (map->data[i] != NULL)
      mcsh_value_drop(logger, map->data[i]);
  strmap_finalize(map);
  free(entry);
}

static void
stack_finalize(mcsh_stack* stack)
{
  mcsh_entry_free(stack->current);
}

void
mcsh_vm_stop(mcsh_vm* vm)
{
  mcsh_log(&vm->logger, MCSH_LOG_CORE, MCSH_DEBUG,
           "VM stop ...");
  mcsh_module_finalize(vm->main);

  mcsh.vms[vm->id] = NULL;
  mcsh.vm_count--;
  stack_finalize(&vm->stack);
  // mcsh_entry_free(vm->entry_main);
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_DEBUG,
           "free globals");
  table_free_callback(&vm->globals, false, vm_global_free, NULL);
  mcsh_data_finalize(vm);
  free(vm->main);
}

static void system_finalize(mcsh_system* mcsh);

void
mcsh_finalize()
{
  char text[64];
  thing_str(mcsh.parse_state.target, text);
  // printf("mcsh_finalize: parse_target is %s\n", text);
  free(mcsh.parse_state.target);
  mcsh_builtins_finalize();
  if (terms_in.size > 0)
    printf("warning: terms_in has size %zi\n", terms_in.size);
  list_array_finalize(&terms_in);
  mcsh_script_lex_destroy();
  system_finalize(&mcsh);
}

static void
system_finalize(mcsh_system* sys)
{
  null(&sys->vms);
}

static void expr_scan_exception(mcsh_status* status);

bool
mcsh_expr_scan(char* code, mcsh_node** node,
               mcsh_status* status)
{
  mcsh_expr_source_set(code);

  mcsh_expr_grammar_status = true;

  // Call to Bison parser:
  mcsh_expr_parse();

  if (! mcsh_expr_grammar_status)
  {
    *node = NULL;
    expr_scan_exception(status);
    return true;  // status is EXCEPTION
  }

  // mcsh_node_print(mcsh.parse_state.output, 0);

  *node = mcsh.parse_state.output;
  return true;
}

static void
expr_scan_exception(mcsh_status* status)
{
  char* source = NULL;
  int line = mcsh_expr_line;
  mcsh_raise(status, source, line,
             "mcsh.syntax_error",
             mcsh_expr_grammar_message);
  null(&mcsh_expr_grammar_message);
}

static bool mcsh_expr_eval_op(mcsh_vm* vm, mcsh_expr* expr,
                              mcsh_value** output);

bool
mcsh_expr_eval(mcsh_vm* vm, mcsh_expr* expr, mcsh_value** output)
{
  // printf("mcsh_expr_eval(%p)\n", expr);
  if (expr == NULL)
  {
    maybe_assign(output, &mcsh_null);
    return true;
  }
  mcsh_value* value = NULL;
  size_t i;
  switch (expr->type)
  {
    case MCSH_EXPR_TYPE_TOKEN:
      // printf("eval: token\n");
      if (output != NULL)
        value = mcsh_value_new_string(vm, expr->children.data[0]);
      break;
    case MCSH_EXPR_TYPE_STMTS:
      // printf("eval: stmts\n");
      if (expr->children.size == 0)
        value = &mcsh_null;
      else
      {
        i = 0;
        if (expr->children.size > 1)
          for (; i < expr->children.size-1; i++)
          {
            // printf("eval: stmts %zi\n", i);
            fflush(stdout);
            mcsh_expr_eval(vm, expr->children.data[i], NULL);
          }
        // printf("eval: stmt root\n");
        fflush(stdout);
        mcsh_expr_eval(vm, expr->children.data[i], &value);
      }
      break;
    case MCSH_EXPR_TYPE_OP:
      mcsh_expr_eval_op(vm, expr, &value);
      break;
  }
  if (output != NULL)
  {
    *output = value;
    char t[64];
    mcsh_to_string(&vm->logger, t, 64, value);
    // printf("eval: %i %s\n", value->type, t);
  }
  /* else */
  /*   printf("eval: NULL\n"); */
  return true;
}

static inline bool eval_binary(mcsh_vm* vm, mcsh_operator op,
                               list_array* operands,
                               int64_t* output);

static inline bool
is_math_op(mcsh_operator op)
{
  return
    op == MCSH_OP_PLUS  ||
    op == MCSH_OP_MINUS ||
    op == MCSH_OP_MULT  ||
    op == MCSH_OP_DIV   ||
    op == MCSH_OP_IDIV  ||
    op == MCSH_OP_MOD   ||
    op == MCSH_OP_EQ    ||
    op == MCSH_OP_NE    ||
    op == MCSH_OP_LT    ||
    op == MCSH_OP_GT    ||
    op == MCSH_OP_LE    ||
    op == MCSH_OP_GE    ||
    op == MCSH_OP_TERN;
}

static inline bool eval_ternary(mcsh_vm* vm, mcsh_operator op,
                                list_array* operands, int64_t* output);


static bool
mcsh_expr_eval_op(mcsh_vm* vm, mcsh_expr* expr, mcsh_value** output)
{
  mcsh_logger* logger = &vm->logger;
  mcsh_operator op = expr->op;
  char t[64];
  op_to_string(t, op);
  LOG(MCSH_LOG_EVAL, MCSH_DEBUG, "eval_op: %s", t);
  int64_t int_result;
  mcsh_value* result;

  if (!is_math_op(op)) valgrind_fail_msg("unknown op: '%s'\n", t);

  if (op == MCSH_OP_TERN)
  {
    eval_ternary(vm, op, &expr->children, &int_result);
  }
  else
  {
    eval_binary(vm, op, &expr->children, &int_result);
  }

  result = mcsh_value_new_int(int_result);
  if (output != NULL)
    *output = result;
  // Use maybe_assign()
  return true;
}

static inline bool eval_binary_raw(mcsh_operator op,
                                   int64_t int_left,
                                   int64_t int_right,
                                   int64_t* output);

static inline bool
eval_binary(mcsh_vm* vm, mcsh_operator op, list_array* operands,
            int64_t* output)
{
  // char t[64];
  mcsh_value* value_left;
  mcsh_value* value_right;
  mcsh_expr_eval(vm, operands->data[0], &value_left);
  // mcsh_to_string(&vm->logger, t, 64, value_left);

  mcsh_expr_eval(vm, operands->data[1], &value_right);
  // mcsh_to_string(logger, t, 64, value_right);

  int64_t int_left, int_right, int_result;
  mcsh_value_integer(value_left,  &int_left);
  mcsh_value_integer(value_right, &int_right);
  eval_binary_raw(op, int_left, int_right, &int_result);
  *output = int_result;

  return true;
}

static inline bool
eval_binary_raw(mcsh_operator op,
                int64_t int_left, int64_t int_right,
                int64_t* output)
{
  int64_t result;
  switch (op)
  {
    case MCSH_OP_PLUS:
      result = int_left + int_right;
      break;
    case MCSH_OP_MINUS:
      result = int_left - int_right;
      break;
    case MCSH_OP_MULT:
      result = int_left * int_right;
      break;
    case MCSH_OP_DIV:
      valgrind_assert_msg(int_right != 0, "mcc: ZERO DIVIDE (/)");
      result = int_left / int_right;
      break;
    case MCSH_OP_IDIV:
      valgrind_assert_msg(int_right != 0, "mcc: ZERO DIVIDE (%/)");
      result = int_left / int_right;
      break;
    case MCSH_OP_MOD:
      valgrind_assert_msg(int_right != 0, "mcc: ZERO DIVIDE (%)");
      result = int_left % int_right;
      /* printf("MOD %"PRId64" %% %"PRId64" -> %"PRId64"\n", */
      /*        int_left, int_right, result); */
      break;
    case MCSH_OP_EQ:
      result = int_left == int_right;
      break;
    case MCSH_OP_NE:
      result = int_left != int_right;
      break;
    case MCSH_OP_LT:
      result = int_left < int_right;
      break;
    case MCSH_OP_GT:
      result = int_left > int_right;
      break;
    case MCSH_OP_LE:
      result = int_left <= int_right;
      break;
    case MCSH_OP_GE:
      result = int_left >= int_right;
      break;
    default:
      valgrind_fail_msg("bad op: %i\n", op);
  }
  *output = result;
  return true;
}

static inline bool
eval_ternary(mcsh_vm* vm, mcsh_operator op, list_array* operands,
             int64_t* output)
{
  valgrind_assert(op == MCSH_OP_TERN);
  mcsh_value* value_condition;
  mcsh_value* value_left;
  mcsh_value* value_right;
  mcsh_expr_eval(vm, operands->data[0], &value_condition);
  mcsh_expr_eval(vm, operands->data[1], &value_left);
  mcsh_expr_eval(vm, operands->data[2], &value_right);

  int64_t int_condition, int_left, int_right, int_result;
  mcsh_value_integer(value_condition,  &int_condition);
  mcsh_value_integer(value_left,       &int_left);
  mcsh_value_integer(value_right,      &int_right);

  // Do it!
  int_result = int_condition ? int_left : int_right;

  *output = int_result;
  return true;
}

bool
mcsh_value_integer(mcsh_value* value, int64_t* output)
{
  char* p;
  switch (value->type)
  {
    case MCSH_VALUE_INT:
    {
      *output = value->integer;
      break;
    }
    case MCSH_VALUE_FLOAT:
    {
      fail("value_integer(): convert from float NYI\n");
      break;
    }
    case MCSH_VALUE_STRING:
    {
      // printf("value_integer(): convert from string\n");
      errno = 0;
      int64_t result = strtod(value->string, &p);
      if (value->string == p || errno != 0)
      {
        printf("invalid integer: '%s'\n", value->string);
        // See exception handling in to_float() below
        return false;
      }
      *output = result;
      break;
    }
    default:
    {
      return false;
    }
  }
  return true;
}

bool
mcsh_to_float(double* result, const mcsh_value* value,
              mcsh_status* status)
{
  int n;
  float f = 0;
  switch (value->type)
  {
    case MCSH_VALUE_INT:
    {
      *result = (double) value->integer;
      break;
    }
    case MCSH_VALUE_FLOAT:
    {
      *result = value->number;
      break;
    }
    case MCSH_VALUE_STRING:
    {
      // printf("to_float(): convert from string\n");
      errno = 0;
      n = sscanf(value->string, "%f", &f);
      *result = f;
      if (n != 1)
      {
        mcsh_raise(status, NULL, 0, "mcsh.number_format",
                   "could not parse float: '%s'", value->string);
      }
      break;
    }
    default:
    {
      return false;
    }
  }
  return true;
}

void
mcsh_expr_token(const char* token)
{
  printf("mcsh_expr_token(%s)\n", token);
}

void
mcsh_expr_end()
{
  printf("mcsh_expr_end()\n");
}

void
mcsh_expr_op(const char* op_string)
{
  printf("mcsh_expr_op(%s)\n", op_string);
}

static inline mcsh_expr*
mcsh_expr_construct(mcsh_expr_type type, size_t size)
{
  mcsh_expr* expr = malloc_checked(sizeof(*expr));
  expr->type = type;
  list_array_init(&expr->children, size);
  return expr;
}

static inline mcsh_expr*
mcsh_expr_construct_token(char* text)
{
  mcsh_expr* expr = mcsh_expr_construct(MCSH_EXPR_TYPE_TOKEN, 1);
  expr->op = MCSH_OP_IDENTITY;
  list_array_add(&expr->children, strdup(text));
  // printf("expr_construct_token: '%s'\n", text);
  // printf("TOKEN c: '%s'\n", (char*) expr->children.data[0]);
  return expr;
}

static inline mcsh_expr*
mcsh_expr_construct_stmts(void)
{
  mcsh_expr* expr = mcsh_expr_construct(MCSH_EXPR_TYPE_STMTS, 1);
  expr->op = MCSH_OP_STMTS;
  return expr;
}

static inline mcsh_expr*
mcsh_expr_construct_op(mcsh_operator op)
{
  mcsh_expr* expr = mcsh_expr_construct(MCSH_EXPR_TYPE_OP, 2);
  expr->op = op;
  // printf("expr_construct_op(): %p\n", expr);
  return expr;
}

static inline void op_to_expr(list_array* ops_node,
                              list_array* ops_expr,
                              int count);

void
mcsh_node_to_expr(mcsh_node* node, mcsh_expr** output)
{
  if (node == NULL)
  {
    // Input was the empty string
    *output = NULL;
    return;
  }
  mcsh_expr* expr;
  mcsh_expr* expr_left;
  mcsh_expr* expr_right;
  mcsh_node* node_left;
  mcsh_node* node_right;
  mcsh_operator* op;

  // printf("node_to_expr() ...\n");
  switch (node->type)
  {
    case MCSH_NODE_TYPE_TOKEN:
      expr = mcsh_expr_construct_token(node->children.data[0]);
      break;
    case MCSH_NODE_TYPE_PAIR:
      expr = mcsh_expr_construct_stmts();
      // printf("children: %zi\n", node->children.size);
      node_left  = node->children.data[0];
      node_right = node->children.data[1];
      mcsh_node_to_expr(node_left,  &expr_left);
      mcsh_node_to_expr(node_right, &expr_right);
      if (expr_left != NULL)
      {
        if (expr_left->type == MCSH_EXPR_TYPE_STMTS)
        {
          // printf("stmts on left\n");
          for (size_t i = 0; i < expr_left->children.size; i++)
            list_array_add(&expr->children,
                           expr_left->children.data[i]);
        }
        else
          list_array_add(&expr->children, expr_left);
        }
      if (expr_right != NULL)
        list_array_add(&expr->children, expr_right);
      // printf("new expr stmts: %p %zi\n", expr, expr->children.size);
      break;
    case MCSH_NODE_TYPE_OP:
      op = node->children.data[0];
      expr = mcsh_expr_construct_op(*op);
      if (*op == MCSH_OP_TERN)
      {
        op_to_expr(&node->children, &expr->children, 3);
      }
      else
      {
        op_to_expr(&node->children, &expr->children, 2);
      }
      break;
    default:
      valgrind_fail_msg("node to expr: illegal type: %i\n",
                        node->type);
      expr = NULL; // suppress uninitialized warning
  }
  *output = expr;
}

static inline void
op_to_expr(list_array* ops_node, list_array* ops_expr, int count)
{
  mcsh_expr* expr;
  for (int i = 1; i <= count; i++)
  {
    mcsh_node_to_expr(ops_node->data[i],  &expr);
    list_array_add(ops_expr, expr);
  }
}

void
mcsh_expr_print(mcsh_expr* expr, int indent)
{
  if (expr == NULL)
  {
    printf("  NULL\n");
    return;
  }

  switch (expr->type)
  {
    case MCSH_EXPR_TYPE_TOKEN:
      // printf("TOKENp: %p\n", expr);
      print_spaces(indent);
      printf("  TOKEN: '%s'\n", (char*) expr->children.data[0]);
      break;
    case MCSH_EXPR_TYPE_OP:
      printf("OPCODE: %i\n", expr->op);
      print_spaces(indent);
      mcsh_expr* left = expr->children.data[0];
      // printf("left: %p\n", left);
      mcsh_expr_print(left, indent+2);
      mcsh_expr_print(expr->children.data[1], indent+2);
      break;
    case MCSH_EXPR_TYPE_STMTS:
      printf("  STMTS\n");
      for (size_t i = 0; i < expr->children.size; i++)
        mcsh_expr_print(expr->children.data[i], indent+2);
      break;
    default:
      valgrind_fail();
  }
}

static inline unsigned int
mcsh_strfromd(char* restrict str, size_t n, double fp)
{
  unsigned int result;
#ifdef HAVE_STRFROMD
  result = strfromd(str, n, "%f", fp);
#else
  // strfromd() is not in Cygwin
  result = snprintf(str, n, "%f", fp);
#endif
  valgrind_assert(result < n);
  return result;
}

size_t
mcsh_to_string(mcsh_logger* logger,
               char* result, size_t max, const mcsh_value* value)
{
  size_t actual = 0;
  valgrind_assert_msg(value != NULL, "value == NULL");
  // printf("result from: %p %p %i\n",
  //      value, value->function, value->type);
  buffer B;
  switch (value->type)
  {
    case MCSH_VALUE_NULL:
      actual = snprintf(result, max, "(NULL)");
      break;
    case MCSH_VALUE_STRING:
      actual = snprintf(result, max, "%s", value->string);
      break;
    case MCSH_VALUE_INT:
      actual = snprintf(result, max, "%"PRId64, value->integer);
      break;
    case MCSH_VALUE_FLOAT:
      actual = mcsh_strfromd(result, max, value->number);
      break;
    case MCSH_VALUE_BLOCK:
      actual = snprintf(result, max, "(BLOCK)");
      break;
    case MCSH_VALUE_FUNCTION:
      // printf("MCSH_VALUE_FUNCTION\n");
      actual = snprintf(result, max, "function:%s()",
                        value->function->name);
      break;
    case MCSH_VALUE_LINK:
      actual = snprintf(result, max, "(LINK)");
      break;
    case MCSH_VALUE_LIST:
      buffer_init(&B, 64);
      mcsh_value_buffer(logger, value, &B);
      valgrind_assert(B.length < max);
      strcpy(result, B.data);
      buffer_finalize(&B);
      break;
    case MCSH_VALUE_TABLE:
      actual = sprintf(result, "table:size=%i", value->table->size);
      break;
    case MCSH_VALUE_MODULE:
      actual = sprintf(result, "(MODULE)");
      break;
    case MCSH_VALUE_ACTIVATION:
      actual = sprintf(result, "(ACTIVATION)");
      break;
    default:
      valgrind_fail_msg("mcsh_to_string: unknown value type: %i\n",
                        value->type);
  }
  valgrind_assert(actual < max);
  return actual;
}

bool
mcsh_value_buffer(mcsh_logger* logger, const mcsh_value* value,
                  buffer* output)
{
  size_t actual = 0;
  valgrind_assert_msg(value != NULL, "value == NULL");
  // printf("result from: %p %p %i\n",
  //      value, value->function, value->type);
  char t[64];
  switch (value->type)
  {
    case MCSH_VALUE_NULL:
      buffer_cat(output, "(NULL)");
      break;
    case MCSH_VALUE_STRING:
      buffer_cat(output, value->string);
      break;
    case MCSH_VALUE_INT:
      actual = sprintf(t, "%"PRId64, value->integer);
      buffer_cat(output, t);
      break;
    case MCSH_VALUE_FLOAT:
      actual = mcsh_strfromd(t, 64, value->number);
      buffer_cat(output, t);
      break;
    case MCSH_VALUE_FUNCTION:
      buffer_catv(output, "function:%s()", value->function->name);
      break;
    case MCSH_VALUE_LINK:
      buffer_cat(output, "(LINK)");
      break;
    case MCSH_VALUE_LIST:
      mcsh_join_list_to_buffer(logger, value->list, ",", output);
      break;
    case MCSH_VALUE_TABLE:
      mcsh_join_table_to_buffer(logger, value->table, ",", output);
      break;
    default:
      valgrind_fail_msg("mcsh_value_buffer: unknown value type: %i\n",
                        value->type);
  }
  return actual;
}

bool
mcsh_code_name(mcsh_code code, char* output)
{
  switch (code)
  {
    case MCSH_OK:        strcpy(output, "OK"       );  break;
    case MCSH_RETURN:    strcpy(output, "RETURN"   );  break;
    case MCSH_BREAK:     strcpy(output, "BREAK"    );  break;
    case MCSH_CONTINUE:  strcpy(output, "CONTINUE" );  break;
    case MCSH_EXIT:      strcpy(output, "EXIT"     );  break;
    case MCSH_EXCEPTION: strcpy(output, "EXCEPTION");  break;
    default:
      valgrind_fail_msg("bad code: %i\n", code);
  }
  return true;
}

static bool type_names_initialized = false;
static lookup_entry type_names[MCSH_TYPE_COUNT];

static inline void
value_names_init(void)
{
  if (type_names_initialized) return;

  // Sync this with mcsh.h enum mcsh_value_type
  lookup_entry L[MCSH_TYPE_COUNT] =
    {{MCSH_VALUE_NULL,       "NULL"      },
     {MCSH_VALUE_STRING,     "string"    },
     {MCSH_VALUE_INT,        "int"       },
     {MCSH_VALUE_FLOAT,      "float"     },
     {MCSH_VALUE_LIST,       "list"      },
     {MCSH_VALUE_TABLE,      "table"     },
     {MCSH_VALUE_BLOCK,      "block"     },
     {MCSH_VALUE_FUNCTION,   "function"  },
     {MCSH_VALUE_MODULE,     "module"    },
     {MCSH_VALUE_LINK,       "link"      },
     {MCSH_VALUE_ACTIVATION, "activation"},
     {MCSH_VALUE_ANY,        "any"       },
     lookup_sentinel
    };

  for (int i = 0; i < MCSH_TYPE_COUNT; i++)
  {
    type_names[i] = L[i];
  }
}

bool
mcsh_value_type_name(mcsh_value_type type, char* output)
{
  value_names_init();
  char* result = lookup_by_code(type_names, type);
  valgrind_assert_msg(result != NULL, "bad type: %i\n", type);
  strcpy(output, result);
  return true;
}

bool
mcsh_value_type_code(char* name, mcsh_value_type* type)
{
  value_names_init();
  printf("value_type_code: '%s'\n", name);
  int result = lookup_by_text(type_names, name);
  valgrind_assert_msg(result > 0, "bad name: '%s'\n", name);
  *type = result;
  return true;
}

void
mcsh_expr_finalize(mcsh_expr* expr)
{
  if (expr == NULL) return;
}

void
mcsh_node_free(mcsh_node* node, int lvl)
{
  switch (node->type)
  {
    case MCSH_NODE_TYPE_TOKEN:
      free(node->children.data[0]);
      break;
    default:
      for (size_t i = 0; i < node->children.size; i++)
      {
        if (node->children.data[i] == NULL)
          continue;
        mcsh_node_free(node->children.data[i], lvl+1);
      }
  }
  list_array_finalize(&node->children);
  free(node);
}

void
mcsh_final_status(bool result, mcsh_status* status, int* exit_status)
{
  if (! result)
  {
    printf("mcsh: internal error!\n");
    exit(EXIT_FAILURE);
  }
  switch (status->code)
  {
    case MCSH_PROTO: break;
    case MCSH_OK:    break;

    case MCSH_EXCEPTION:
    {
      printf("mcsh: unhandled exception!\n");
      *exit_status = EXIT_FAILURE;
      printf("mcsh: exception tag:  %s\n", status->exception->tag);
      printf("mcsh: exception text: %s\n", status->exception->text);
      mcsh_exception_free(status->exception);
      break;
    }
    case MCSH_EXIT:
    {
      *exit_status = status->value->integer;
      mcsh_log(&mcsh.logger, MCSH_LOG_SYSTEM, MCSH_TRACE,
               "exit status handled: code=%i\n", exit_status);
      break;
    }
    default:
    {
      printf("mcsh: weird exit case: %i\n", status->code);
    }
  }
}
