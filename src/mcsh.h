
/**
   MCSH H
*/

#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1  // for asprintf(), vasprintf()
#endif
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "list-array.h"
#include "list_i.h"
#include "log.h"
#include "strmap.h"
#include "table.h"

typedef struct mcsh_stmts mcsh_stmts;
typedef struct mcsh_module_s mcsh_module;

struct mcsh_stmts
{
  /// Contains items of mcsh_stmt:
  list_array stmts;
};

typedef struct
{
  /// Starting line number:
  int id;
  int line;
  mcsh_stmts stmts;
} mcsh_block;

typedef enum
{
  /// Normal function:
  MCSH_FN_NORMAL,
  /// Hygeinic
  MCSH_FN_INPLACE,
  /// Unhygeinic
  MCSH_FN_MACRO
} mcsh_fn_type;

typedef struct mcsh_signature_s mcsh_signature;

/* Sync this with mcsh.c type_names[] */
/// Number of named types (size of enum + sentinel)
#define MCSH_TYPE_COUNT 13
typedef enum
{
  MCSH_VALUE_NULL        =  0,
  MCSH_VALUE_STRING      =  1,
  MCSH_VALUE_INT         =  2,
  MCSH_VALUE_FLOAT       =  3,
  MCSH_VALUE_LIST        =  4,
  MCSH_VALUE_TABLE       =  5,
  MCSH_VALUE_BLOCK       =  6,
  MCSH_VALUE_FUNCTION    =  7,
  MCSH_VALUE_MODULE      =  8,
  MCSH_VALUE_LINK        =  9,
  MCSH_VALUE_ACTIVATION  =  10,
  MCSH_VALUE_ANY         =  1000
} mcsh_value_type;

bool mcsh_value_type_name(mcsh_value_type type, char* output);
bool mcsh_value_type_code(char* name, mcsh_value_type* type);

typedef enum
{
  MCSH_PROTO,
  MCSH_OK,
  MCSH_RETURN,
  /** loop break */
  MCSH_BREAK,
  /** loop continue */
  MCSH_CONTINUE,
  /** user issued exit command */
  MCSH_EXIT,
  /** user code triggered exception */
  MCSH_EXCEPTION
} mcsh_code;

typedef struct mcsh_value_s     mcsh_value;
typedef struct mcsh_exception_s mcsh_exception;

/** Resulting status from a function call */
typedef struct
{
  mcsh_code code;
  mcsh_exception* exception;
  mcsh_value* value;
} mcsh_status;

typedef struct mcsh_function_s mcsh_function;

typedef struct mcsh_entry_s mcsh_entry;

typedef bool (*mcsh_activation_get)(mcsh_entry* entry,
                                    const char* name,
                                    mcsh_value** output,
                                    mcsh_status* status);
typedef bool (*mcsh_activation_set)(mcsh_entry* entry,
                                    const char* name,
                                    mcsh_value* input,
                                    mcsh_status* status);

typedef struct
{
  mcsh_activation_get get;
  mcsh_activation_set set;
} mcsh_activation;

struct mcsh_value_s
{
  mcsh_value_type type;
  int refs;
  bool word_split;
  union
  {
    char* string;
    int64_t integer;
    double number;
    list_array* list;
    struct table* table;
    mcsh_block* block;
    mcsh_function* function;
    mcsh_module* module;
    mcsh_value* link;
    mcsh_activation* activation;
  };
};

typedef struct
{
  char*           name;
  mcsh_value*     dflt;
  mcsh_value_type type;
} mcsh_slot;

struct mcsh_signature_s
{
  uint16_t   count;
  /// Are extra arguments allowed (...) ?
  bool       extras;
  mcsh_slot* slots;
};

struct mcsh_function_s
{
  mcsh_fn_type   type;
  char*          name;
  mcsh_signature signature;
  mcsh_block*    block;
};

typedef struct mcsh_stmt mcsh_stmt;

typedef enum
{
  MCSH_OP_IDENTITY = 1,
  MCSH_OP_STMTS    = 2,
  MCSH_OP_PLUS     = 3,
  MCSH_OP_MINUS    = 4,
  MCSH_OP_MULT     = 5,
  MCSH_OP_DIV      = 6,
  MCSH_OP_IDIV     = 7,
  MCSH_OP_MOD      = 8,
  MCSH_OP_EQ       = 100,
  MCSH_OP_NE       = 101,
  MCSH_OP_LT       = 102,
  MCSH_OP_GT       = 103,
  MCSH_OP_LE       = 104,
  MCSH_OP_GE       = 105,
  MCSH_OP_TERN     = 200
} mcsh_operator;

static UNUSED void
op_to_string(char* s, mcsh_operator op)
{
  switch (op)
  {
    case MCSH_OP_PLUS:
      strcpy(s, "+");
      break;
    case MCSH_OP_MINUS:
      strcpy(s, "-");
      break;
    case MCSH_OP_MULT:
      strcpy(s, "*");
      break;
    case MCSH_OP_DIV:
      strcpy(s, "/");
      break;
    case MCSH_OP_IDIV:
      strcpy(s, "%/");
      break;
    case MCSH_OP_MOD:
      strcpy(s, "%");
      break;
    case MCSH_OP_EQ:
      strcpy(s, "==");
      break;
    case MCSH_OP_NE:
      strcpy(s, "!=");
      break;
    case MCSH_OP_LT:
      strcpy(s, "<");
      break;
    case MCSH_OP_GT:
      strcpy(s, ">");
      break;
    case MCSH_OP_LE:
      strcpy(s, "<=");
      break;
    case MCSH_OP_GE:
      strcpy(s, ">=");
      break;
    case MCSH_OP_TERN:
      strcpy(s, "?:");
      break;
    default:
      valgrind_fail_msg("bad op: %i\n", op);
  }
}

typedef struct
{
  char* text;
} mcsh_token;

typedef struct
{
  mcsh_stmts stmts;
} mcsh_subcmd;

typedef struct
{
  mcsh_stmts stmts;
} mcsh_subfun;

typedef enum
{
  MCSH_THING_NULL,
  MCSH_THING_TOKEN,
  MCSH_THING_STMT,
  MCSH_THING_BLOCK,
  /** Substitute command: $(( cmd )) */
  MCSH_THING_SUBCMD,
  /** Substitute function: (( f $x )) */
  MCSH_THING_SUBFUN,
  MCSH_THING_MODULE
} mcsh_thing_type;

typedef union
{
  mcsh_token*  token;
  mcsh_stmt*   stmt;
  mcsh_block*  block;
  mcsh_subcmd* subcmd;
  mcsh_subfun* subfun;
  mcsh_module* module;
} mcsh_thing_data;

/// A Thing is a Token, Stmt, Block, SubCmd, SubFun, or Module
typedef struct mcsh_thing_s mcsh_thing;

struct mcsh_thing_s
{
  mcsh_thing_type type;
  mcsh_thing_data data;
  mcsh_thing*     parent;
  mcsh_module*    module;
};

struct mcsh_stmt
{
  mcsh_module* module;
  mcsh_thing* parent;
  /** contains items of mcsh_thing */
  list_array things;
  /// Line number in the user script
  int line;
};

typedef struct
{
  // May be NULL:
  mcsh_value* name;
  mcsh_value* value;
} mcsh_arg;

static inline void
mcsh_arg_init(mcsh_arg* arg, mcsh_value* name, mcsh_value* value)
{
  arg->name  = name;
  arg->value = value;
}

void mcsh_value_grab(mcsh_logger* logger, mcsh_value* value);

void mcsh_signature_init(mcsh_signature* sg, uint16_t count,
                         char** names, mcsh_value** dflts,
                         bool extras);

void mcsh_signature_print(mcsh_signature* sg);

void mcsh_signature_finalize(mcsh_signature* sg);

typedef struct
{
  uint16_t     count;
  char**       names;
  mcsh_value** values;
  list_array   extra_names;
  list_array   extra_values;
} mcsh_parameters;

/** sg : IN , L : IN , P : OUT
    Assigns functions arguments in L to signature sg,
    resulting in parameters P
 */
bool mcsh_parameterize(mcsh_signature* sg, list_array* L,
                       mcsh_parameters* P, mcsh_status* status);

bool mcsh_parameters_print(mcsh_parameters* P);

void mcsh_parameters_finalize(mcsh_parameters* P);

extern mcsh_value mcsh_null;

size_t mcsh_to_string(mcsh_logger* logger,
                      char* result, size_t max,
                      const mcsh_value* value);

/** Return true unless could not convert to int, then false */
bool mcsh_value_integer(mcsh_value* value, int64_t* output);

bool mcsh_to_float(double* result,
                   const mcsh_value* value,
                   mcsh_status* status);

bool mcsh_value_buffer(mcsh_logger* logger, const mcsh_value* value,
                       buffer* output);

/// We assign numbers for debugging
typedef enum
{
  MCSH_NODE_TYPE_NONE   = 0,
  MCSH_NODE_TYPE_TOKEN  = 1,
  MCSH_NODE_TYPE_OP     = 2,
  MCSH_NODE_TYPE_PAIR   = 3,
  MCSH_NODE_TYPE_STMTS  = 4,
  MCSH_NODE_TYPE_BLOCK  = 5,
  MCSH_NODE_TYPE_SUBCMD = 6,
  MCSH_NODE_TYPE_SUBFUN = 7
} mcsh_node_type;

typedef struct
{
  mcsh_node_type type;
  /** children are:
      if   type==TOKEN: char*
      else node
  */
  list_array children;
  /// Line number in user script
  int line;
} mcsh_node;

typedef enum
{
  MCSH_EXPR_TYPE_TOKEN,
  MCSH_EXPR_TYPE_OP,
  MCSH_EXPR_TYPE_STMTS
} mcsh_expr_type;

typedef struct
{
  mcsh_expr_type type;
  mcsh_operator op;
  list_array children;
} mcsh_expr;

typedef enum
{
  MCSH_PARSE_NONE,
  MCSH_PARSE_START,
  MCSH_PARSE_OK,
  MCSH_PARSE_FAIL
} mcsh_parse_status;

typedef struct
{
  /// Source filename or equivalent for parser text:
  char source[PATH_MAX];
  /// Symbolic name for parser text:
  char name[PATH_MAX];
  mcsh_parse_status status;
  mcsh_thing* target;
  /// Resulting node tree goes here:
  mcsh_node* output;
  int id;
} mcsh_parse_state;

typedef struct mcsh_vm_s    mcsh_vm;
typedef struct mcsh_stack_s mcsh_stack;
typedef struct mcsh_data_s  mcsh_data;

bool mcsh_signature_parse(mcsh_module* module,
                          mcsh_signature* signature,
                          mcsh_block* sgtokens,
                          mcsh_status* status);

struct mcsh_data_s
{
  struct table* specials;
};

struct mcsh_stack_s
{
  mcsh_vm* vm;
  mcsh_entry* current;
};

struct mcsh_vm_s
{
  int id;
  /** Map from char* to mcsh_value* */
  struct table globals;
  unsigned int argc;
  char** argv;
  /** List of directories to search for imports
      Searches start at the end!
   */
  list_array path;
  mcsh_module* main;
  mcsh_data* data;
  mcsh_stack stack;
  mcsh_entry* entry_main;
  struct list_i jobs;
  mcsh_logger logger;
  int exit_code_last;
};

struct mcsh_module_s
{
  mcsh_vm* vm;
  /// Source filename or equivalent for parser text
  char source[PATH_MAX];
  char name[PATH_MAX];
  // mcsh_parse_target parse_target;
  strmap vars;
  /// The next stmt to execute
  /// Note that this is only used for the interactive stdin module
  int instruction;
  mcsh_stmts stmts;
  mcsh_module* parent;
};

typedef enum
{
  // A module.  Not really an entry.
  MCSH_ENTRY_MODULE,
  // A normal stack frame
  MCSH_ENTRY_FRAME,
  // A sub-scope within a frame for inplace
  MCSH_ENTRY_SCOPE,
  // A sub-scope within a frame for macros
  MCSH_ENTRY_MACRO
} mcsh_entry_type;

struct mcsh_entry_s
{
  uint64_t id;
  mcsh_entry_type type;
  mcsh_stack* stack;
  mcsh_entry* parent;
  int64_t depth;
  strmap vars;
  int shift;
  // Pointer because this may be an alias
  list_array* args;
  // Only used if this is an ENTRY_MODULE:
  mcsh_module* module;
};

bool mcsh_code_name(mcsh_code type, char* output);

struct mcsh_exception_s
{
  char* tag;
  mcsh_exception* cause;
  char* text;

  // Source:
  char* source;
  int   line;
};

typedef struct
{
  pid_t pid;
  mcsh_vm** vms;
  /// Capacity of vms:
  size_t vm_capacity;
  /// Number of running VMs:
  size_t vm_count;
  mcsh_parse_state parse_state;
  struct table* builtins;
  struct table* exprs;
  mcsh_logger logger;
} mcsh_system;

extern mcsh_system mcsh;

typedef enum
{
  MCSH_MODE_PROTO,
  MCSH_MODE_INTERACTIVE,
  MCSH_MODE_SCRIPT,
  MCSH_MODE_STRING
} mcsh_mode;

typedef struct
{
  char mcsh_command[128];
  FILE* stream;
  mcsh_mode mode;
  unsigned int argc;
  /**
     If MODE_INTERACTIVE: unused
     If MODE_STRING:
     argv contains each -c argument
     If MODE_SCRIPT:
     argv[0] is the name of the script (or "stdin")
     The rest are arguments to the script
  */
  char** argv;
  strmap globals;
} mcsh_cmd_line;

#include "mcsh-data.h"

bool mcsh_init(void);

void mcsh_status_init(mcsh_status* status);

void mcsh_cmd_line_init(mcsh_cmd_line* cmd);

/** Handle the options to mcsh */
bool mcsh_parse_options(unsigned int argc, char* argv[],
                        mcsh_cmd_line* cmd);

/** Handle the arguments to the user script */
bool mcsh_parse_args(unsigned int argc, char* argv[],
                     mcsh_cmd_line* cmd);

void mcsh_handle_cmd_string(list_array* cmd_tmp,
                            mcsh_cmd_line* cmd);

void mcsh_cmd_line_finalize(mcsh_cmd_line* cmd);

void mcsh_vm_init(mcsh_vm* vm);

void mcsh_vm_init_argv(mcsh_vm* vm, int argc, char** argv);

void mcsh_vm_init_cmd(mcsh_vm* vm, mcsh_cmd_line* cmd);

void mcsh_module_init(mcsh_module* module, mcsh_vm* vm);

void mcsh_stack_init(mcsh_stack* stack, mcsh_vm* vm);

static inline mcsh_vm*
mcsh_entry_vm(mcsh_entry* entry)
{
  return entry->stack->vm;
}

bool mcsh_stack_search(mcsh_entry* entry, const char* name,
                       mcsh_value** result);

void mcsh_entry_init_module(mcsh_entry* entry,
                            mcsh_module* module,
                            mcsh_entry* parent);

bool mcsh_module_parse(const char* source,
                       const char* name,
                       char* code,
                       mcsh_module* module);

void mcsh_module_print(mcsh_module* module, int indent);

/** Log a new line of code */
void mcsh_log_line(mcsh_module* module, const char* code);

bool mcsh_import(mcsh_module* caller, const char* name,
                 mcsh_value** output, mcsh_status* status);

bool mcsh_source(mcsh_module* caller, const char* name,
                 mcsh_value** output, mcsh_status* status);

bool mcsh_eval(mcsh_module* caller, char* code,
               mcsh_value** output, mcsh_status* status);

void mcsh_stmt_print(mcsh_stmt* stmt, int indent);

void mcsh_value_free(mcsh_logger* logger, mcsh_value* value);

void mcsh_value_drop(mcsh_logger* logger, mcsh_value* value);

void mcsh_thing_show(mcsh_thing* thing, int indent);

mcsh_thing* mcsh_thing_from_value(mcsh_module* module,
                                  mcsh_value* value);

void mcsh_block_print(mcsh_block* block, int indent);

void mcsh_subcmd_print(mcsh_subcmd* subst, int indent);

void mcsh_subfun_print(mcsh_subfun* subst, int indent);

void mcsh_token_print(mcsh_token* token);

/* void mcsh_stmt_end2(void); */

void mcsh_block_start(void);

void mcsh_block_end2(void);

void mcsh_subcmd_start(void);

void mcsh_subfun_start(void);

void mcsh_raise(mcsh_status* status, char* source, int line,
                const char* tag, const char* fmt, ...);

/** Use when line/source are unknown */
void mcsh_raise0(mcsh_status* status,
                 const char* tag, const char* fmt, ...);

void mcsh_exception_free(mcsh_exception* exception);

void mcsh_exception_reset(mcsh_status* status);

void mcsh_module_end(void);

/** HP: Hypertext Processing */
void mcsh_hp(mcsh_vm* vm, const char* text);

static inline void
mcsh_value_init(mcsh_value* value)
{
  // TODO: Move type assignment in here for compactness
  //       Short job when have clean SVN for testing
  value->refs       = 0;
  value->word_split = false;
}

static inline void
mcsh_value_init_null(mcsh_value* value)
{
  mcsh_value_init(value);
  value->type = MCSH_VALUE_NULL;
  value->link = &mcsh_null;
}

static inline void
mcsh_value_init_string(mcsh_value* value, char* string)
{
  mcsh_value_init(value);
  value->type = MCSH_VALUE_STRING;
  value->string = string;
}

static inline void
mcsh_value_init_int(mcsh_value* value, int64_t i)
{
  mcsh_value_init(value);
  value->type    = MCSH_VALUE_INT;
  value->integer = i;
}

static inline void
mcsh_value_init_float(mcsh_value* value, double number)
{
  mcsh_value_init(value);
  value->type   = MCSH_VALUE_FLOAT;
  value->number = number;
}

static inline void
mcsh_value_init_list_sized(mcsh_value* value, size_t size)
{
  mcsh_value_init(value);
  value->type = MCSH_VALUE_LIST;
  value->list = list_array_construct(size);
}

static inline void
mcsh_value_init_list(mcsh_value* value)
{
  mcsh_value_init_list_sized(value, 4);
}

static inline void
mcsh_value_init_table(mcsh_value* value, size_t size)
{
  mcsh_value_init(value);
  value->type  = MCSH_VALUE_TABLE;
  value->table = table_create(size);
}

static inline void
mcsh_value_init_module(mcsh_value* value, mcsh_module* module)
{
  mcsh_value_init(value);
  value->type   = MCSH_VALUE_MODULE;
  value->module = module;
}

mcsh_value* mcsh_value_new_null(void);
mcsh_value* mcsh_value_new_link(mcsh_value* value);
mcsh_value* mcsh_value_new_int(int64_t i);
mcsh_value* mcsh_value_new_float(double f);
mcsh_value* mcsh_value_new_string(mcsh_vm* vm, const char* s);
mcsh_value* mcsh_value_new_string_null(void);
mcsh_value* mcsh_value_new_list(mcsh_vm* vm);
mcsh_value* mcsh_value_new_list_sized(mcsh_vm* vm, size_t size);
mcsh_value* mcsh_value_new_table(mcsh_vm* vm, size_t size);
mcsh_value* mcsh_value_new_module(mcsh_vm* vm,
                                  mcsh_module* module);
mcsh_value* mcsh_value_new_function(mcsh_module* module,
                                    mcsh_fn_type type,
                                    const char* name,
                                    mcsh_block* arglist,
                                    mcsh_block* code,
                                    mcsh_status* status);

mcsh_value* mcsh_value_new_activation(mcsh_activation* activation);

mcsh_value* mcsh_value_clone(mcsh_value* value);

void mcsh_value_assign(mcsh_value* target, mcsh_value* value);

void mcsh_table_add(mcsh_logger* logger,
                    mcsh_value* table,
                    mcsh_value* key, mcsh_value* value);

bool mcsh_module_execute(mcsh_module* module,
                         mcsh_value** output,
                         mcsh_status* status);

bool mcsh_stmts_execute(mcsh_module* module, mcsh_stmts* stmts,
                        mcsh_value** output, mcsh_status* status);


/** Parser insertion point */
bool mcsh_module_put_token(mcsh_module* module, char* token);

/** Parser insertion point */
bool mcsh_module_end_stmt(mcsh_module* module, char* token);

bool mcsh_expr_scan(char* code, mcsh_node** node, mcsh_status* status);

void mcsh_node_to_expr(mcsh_node* node, mcsh_expr** output);

void mcsh_expr_print(mcsh_expr* expr, int indent);

bool mcsh_expr_eval(mcsh_vm* vm, mcsh_expr* expr, mcsh_value** output);

void mcsh_expr_finalize(mcsh_expr* expr);

void mcsh_entry_free(mcsh_entry* entry);

void mcsh_module_finalize(mcsh_module* module);

void mcsh_vm_stop(mcsh_vm* vm);

void mcsh_node_free(mcsh_node* node, int lvl);

/** Final stop to handle exceptions and errors
    This function cannot return an error,
    there are no more error handlers!
    status IN, exit_status OUT
 */
void mcsh_final_status(bool result, mcsh_status* status,
                       int* exit_status);

void mcsh_finalize(void);
