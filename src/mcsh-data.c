
/**
   MCSH DATA
*/

#define _GNU_SOURCE  // for strchrnul()
#include <assert.h>
#include <glob.h>
#include <regex.h>
#include <string.h>

#include "activations.h"
#include "exceptions.h"
// #include "strlcpy.h"
#include "mcsh-data.h"
#include "util.h"

static void data_init_activations(mcsh_vm* vm);

void
mcsh_data_init(mcsh_vm* vm)
{
  vm->data = malloc_checked(sizeof(mcsh_data));
  vm->data->specials = table_create(32);
  data_init_activations(vm);
}

static void
data_init_activations(mcsh_vm* vm)
{
  mcsh_activation_init(&vm->logger, vm->data->specials);
}

typedef struct
{
  mcsh_logger* logger;
  mcsh_entry* entry;
  mcsh_status* status;
} context;

bool
mcsh_set_value(mcsh_module* module,
               const char* name, mcsh_value* value,
               mcsh_status* status)
{
  mcsh_logger* logger = &module->vm->logger;

  valgrind_assert(module->vm != NULL);
  valgrind_assert(module->vm->stack.current != NULL);
  mcsh_entry* entry = module->vm->stack.current;

  context ctx;
  ctx.logger = logger;
  ctx.entry = entry;
  ctx.status = status;

  LOG(MCSH_LOG_DATA, MCSH_DEBUG,
      "set_value(): entry=%zi:%zi name='%s'",
      entry->depth, entry->id, name);
  valgrind_assert_msg(entry != NULL, "Stack entry is NULL!");
  bool modules_only = false;
  // A prior value:
  void*  old;
  void** oldp = &old;
  while (true)
  {
    if (modules_only)
      if (entry->type != MCSH_ENTRY_MODULE)
        goto loop;
    size_t index;
    if (strmap_search_index(&entry->vars, name, &index))
    {
      // void* old = strmap_get_index(&entry->vars, index);
      // TODO: free old
      LOG(MCSH_LOG_DATA, MCSH_TRACE,
          "set_value: found in entry=%zi:%zi @%zi '%s'\n",
          entry->depth, entry->id, index, name);
      mcsh_value* existing = strmap_get_value(&entry->vars, index);
      if (existing->type == MCSH_VALUE_LINK)
      {
        LOG(MCSH_LOG_DATA, MCSH_TRACE, "LINK");
        mcsh_value_assign(existing->link, value);
      }
      else
      {
        mcsh_value_drop(&module->vm->logger, existing);
        strmap_set_value(&entry->vars, index, value);
        mcsh_value_grab(&module->vm->logger, value);
      }
      goto found;
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
  if (table_search(&module->vm->globals, name, NULL))
  {
    bool existed = table_set(&module->vm->globals, name, value, oldp);
    if (existed) mcsh_value_drop(&module->vm->logger, old);
    mcsh_value_grab(&module->vm->logger, value);
    goto found;
  }
  if (table_search(module->vm->data->specials, name, oldp))
  {
    LOG(MCSH_LOG_DATA, MCSH_INFO, "set special!");
    mcsh_value* target = old;
    if (target->type == MCSH_VALUE_ACTIVATION)
    {
      LOG(MCSH_LOG_DATA, MCSH_INFO, "set activation!");
      target->activation->set(ctx.entry, "random", value, ctx.status);
    }
    else
    {
      mcsh_value_drop(&module->vm->logger, old);
      table_set(module->vm->data->specials,
                name, value, NULL);
    }
    goto found;
  }
  // not found yet - store in local entry
  LOG(MCSH_LOG_DATA, MCSH_INFO,
      "variable not found: creating local: '%s'", name);
  // printf("vm: %p\n", module->vm);
  // printf("vm stack current: %p\n", module->vm->stack.current);
  // printf("  in entry: %zi %p\n", module->vm->stack.current->id,
     //    module->vm->stack.current);

  strmap_add(&module->vm->stack.current->vars, name, value);
  mcsh_value_grab(&module->vm->logger, value);
  // module->vm->stack.current
  found:
  return true;
}

bool
mcsh_drop_variable(mcsh_module* module,
                   const char* name,
                   UNUSED mcsh_status* status)
{
  mcsh_entry* entry = module->vm->stack.current;
  bool modules_only = false;
  size_t index;
  while (true)
  {
    if (modules_only)
      if (entry->type != MCSH_ENTRY_MODULE)
        goto loop;
    if (strmap_search_index(&entry->vars, name, &index))
    {
      strmap_drop_index(&entry->vars, index);
      /* printf("stack_search(): %zi:%zi found:  '%s'\n", */
      /*        entry->depth, entry->id, name); */
      goto found;
    }
    if (entry->type == MCSH_ENTRY_MODULE)
    {
      if (strmap_search_index(&entry->module->vars, name, &index))
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
  if (table_contains(&vm->globals, name))
  {
    table_remove(&vm->globals, name, NULL);
    goto found;
  }

  if (mcsh_drop_env(vm, name))
    goto found;

  // not found
  return false;

  found:
  return true;
}

static void mcsh_stack_print_entry(mcsh_module* module,
                                   mcsh_entry* entry);

void
mcsh_stack_print(mcsh_module* module)
{
  valgrind_assert(module->vm != NULL);
  valgrind_assert(module->vm->stack.current != NULL);
  mcsh_stack_print_entry(module, module->vm->stack.current);
}

static void
mcsh_stack_print_entry(mcsh_module* module, mcsh_entry* entry)
{
  if (entry->parent != NULL)
    mcsh_stack_print_entry(module, entry->parent);
  printf("entry: %zi:%zi\n", entry->depth, entry->id);
  strmap_show_text(&entry->vars);
  printf("\n");
}

static void data_value_free(void* context, const char* name, void* v);

void
mcsh_data_finalize(mcsh_vm* vm)
{
  // printf("data specials: size: %i\n", table_size(vm->data->specials));
  table_free_callback(vm->data->specials, true,
                      data_value_free, &vm->logger);
  free(vm->data);
}

static void
data_value_free(void* context, UNUSED const char* name, void* v)
{
  mcsh_logger* logger = context;
  mcsh_value*  value  = v;
  mcsh_value_free(logger, value);
}

void
mcsh_assign_specials(mcsh_vm* vm, strmap* parameters)
{
  for (size_t i = 0; i < parameters->size; i++)
  {
    char*       key   = strmap_get_key  (parameters, i);
    mcsh_value* value = strmap_get_value(parameters, i);
    mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_INFO,
             "assign specials: %"PRIu64" '%s' %i %p",
             i, key, value->type, value);

    table_add(vm->data->specials, key, value);
    mcsh_value_grab(&vm->logger, value);
  }
}

typedef enum
{
  CONTIG_INTEGERS,
  CONTIG_STRINGS
} contig_type;

typedef struct
{
  contig_type type;
  union
  {
    // CONTIG_INTEGERS:
    struct
    {
      uint64_t start;
      uint64_t end;
      uint64_t stride;
      unsigned int is_range   : 1;
      unsigned int start_set  : 1;
      unsigned int end_set    : 1;
      unsigned int stride_set : 1;
    };
    // CONTIG_STRINGS:
    // May be a list of keys in the future
    char* key;
  };
} contig;

typedef struct
{
  /// List of contig*
  list_array contigs;
} subscript;

typedef enum
{
  EXPANDER_NONE = 0, /// $v
  EXPANDER_SIZE,     /// $#v
  EXPANDER_TEST,     /// $+v
  EXPANDER_VIEW,     /// $@v
  EXPANDER_REGEX     /// $v/regex/subst
} expander_type;

typedef struct
{
  expander_type type;
  /// Text to go with the expander (regex/subst)
  char* text;
} expander;

typedef enum
{
  VARIABLE_PROTO  = 0,  /// unset value
  VARIABLE_SCALAR = 1,
  VARIABLE_LIST   = 2,
  VARIABLE_TABLE  = 3,
  VARIABLE_MODULE = 4
} variable_type;

typedef struct
{
  variable_type type;
  char name[128];
  /// For modules:
  char* subname;
  /// For subscript expansions: e.g., $L[2:3,4]
  bool subscripted;
  subscript subscript;
  /// For expansion behavior, e.g., $#s
  expander expander;
} variable;

static inline bool variable_parse(context* ctx,
                                  variable* v,
                                  const char* s);

static bool subscript_eval(context* ctx, variable* v,
                           mcsh_value* value,
                           mcsh_value** output);

static bool variable_eval(mcsh_entry* entry,
                          variable* v, mcsh_value* value,
                          mcsh_value** subvalue);

static mcsh_value* expand(mcsh_vm* vm,
                          variable* v, mcsh_value* value);

static mcsh_value* expand_test(bool found);

static mcsh_value* activate(context* ctx,
                            const char* name,
                            mcsh_value* value);

static bool to_value(context* ctx, const char* token,
                     mcsh_value** output);

/** External entry point */
bool
mcsh_token_to_value(mcsh_logger* logger,
                    mcsh_entry* entry, const char* token,
                    mcsh_value** output, mcsh_status* status)
{
  context ctx;
  ctx.logger = logger;
  ctx.entry = entry;
  ctx.status = status;
  LOG(MCSH_LOG_DATA, MCSH_INFO, "token_to_value: '%s'", token);
  return to_value(&ctx, token, output);
}

static bool arg_index(size_t index, context* ctx,
                      mcsh_value** output);
static bool arg_all(context* ctx,
                    mcsh_value** output);

static inline bool is_glob(const char* token);
static bool do_glob(context* ctx, const char* token,
                    mcsh_value** output);

static bool
to_value(context* ctx, const char* token, mcsh_value** output)
{
  mcsh_logger* logger = &ctx->entry->module->vm->logger;
  mcsh_value* value = NULL;
  bool rc;
  if (token[0] == '$' && token[1] != '\0')
  {
    variable v;
    variable_parse(ctx, &v, &token[1]);
    LOG(MCSH_LOG_DATA, MCSH_DEBUG, "name:  '%s' type=%i",
        v.name, v.type);
    size_t index;
    if (is_integer(v.name, &index))
    {
      arg_index(index, ctx, &value);
      PROPAGATE(ctx->status);
      rc = true;
    }
    else if (strcmp(v.name, "*") == 0)
    {
      list_array* A = ctx->entry->args;
      value = mcsh_value_new_int(A->size - 1);
      rc = true;
    }
    else if (strcmp(v.name, "@") == 0)
    {
      arg_all(ctx, &value);
      rc = true;
    }
    else
    {
      rc = mcsh_stack_search(ctx->entry, v.name, &value);
    }
    if (v.expander.type == EXPANDER_TEST &&
        ! v.subscripted)
    {
      value = expand_test(rc);
      goto end;
    }

    RAISE_IF(!rc, ctx->status, NULL, 0, "mcsh.undefined",
             "could not find variable '%s'", v.name);

    value = activate(ctx, v.name, value);
    // printf("variable found: '%s'\n", v.name);
    if (v.type != VARIABLE_SCALAR)
    {
      mcsh_value* subvalue;
      rc = variable_eval(ctx->entry, &v, value, &subvalue);
      CHECK(rc, "could not eval variable: '%s'", token);
      value = subvalue;
    }
    if (v.subscripted)
    {
      mcsh_value* t = NULL;
      rc = subscript_eval(ctx, &v, value, &t);
      PROPAGATE(ctx->status);
      value = t;
    }
    if (v.expander.type != EXPANDER_NONE)
      value = expand(ctx->entry->stack->vm, &v, value);
  }
  else if (is_glob(token))
  {
    rc = do_glob(ctx, token, &value);
    valgrind_assert(rc);
  }
  /*
    else if (strchr(&token[1], '=') != NULL)  // Skip leading =
  {
    // Return PAIR
    // Why return pair? 2025-09-17
    valgrind_fail_msg("NYI");
  }
  */
  else
  {
    value = mcsh_value_new_string(ctx->entry->module->vm, token);
  }
  end:
  *output = value;
  return true;
}

static bool
arg_index(size_t index, context* ctx,
          mcsh_value** output)
{
  int shift = ctx->entry->shift;
  show("size: %zi", ctx->entry->args->size);
  *output = ctx->entry->args->data[index+shift];
  return true;
}

static bool
arg_all(context* ctx, mcsh_value** output)
{
  mcsh_logger* logger = &ctx->entry->module->vm->logger;
  LOG(MCSH_LOG_DATA, MCSH_DEBUG, "arg_all()");
  int shift = ctx->entry->shift;
  list_array* A = ctx->entry->args;
  size_t n = A->size - 1 - shift;
  printf("n: %zi\n", n);
  mcsh_value* result =
    mcsh_value_new_list_sized(ctx->entry->stack->vm, n);
  for (size_t index = shift + 1; index < A->size; index++)
    list_array_add(result->list, A->data[index]);
  *output = result;
  return true;
}

static inline bool
is_glob(const char* token)
{
  char* p = strchr(token, '*');
  return (p != NULL);
}

/* DIR* dir = opendir("."); */
/* if (dir == NULL) */
/* { */
/*   RAISE(ctx->status, "FNF", "could not open directory"); */
/* } */

/* while (true) */
/* { */
/*   struct dirent* E = readdir(dir); */
/*   if (E == NULL) break; */
/*   printf("dirent: %s\n", E->d_name); */
/*   glob(token,  */
/* } */

static bool
do_glob(context* ctx, const char* token, mcsh_value** output)
{
  glob_t G;
  mcsh_vm* vm = ctx->entry->stack->vm;
  // |GLOB_TILDE is not on Cygwin
  glob(token, GLOB_ERR|GLOB_MARK, NULL, &G);
  printf("matches: %zi\n", G.gl_pathc);
  mcsh_value* result = mcsh_value_new_list_sized(vm,
                                                       G.gl_pathc);
  for (size_t i = 0; i < G.gl_pathc; i++)
  {
    mcsh_value* v = mcsh_value_new_string(vm, G.gl_pathv[i]);
    mcsh_value_grab(ctx->logger, v);
    list_array_add(result->list, v);
  }
  result->word_split = true;
  *output = result;
  return true;
}

static mcsh_value*
activate(context* ctx, const char* name, mcsh_value* value)
{
  if (value->type != MCSH_VALUE_ACTIVATION)
    return value;
  mcsh_value* result;
  value->activation->get(ctx->entry, name, &result, ctx->status);
  // mcsh_value_grab(logger, value);
  return result;
}

static mcsh_value*
expand_test(bool found)
{
  return mcsh_value_new_int(found);
}

static void
variable_print(variable* v)
{
  printf("variable '%s'\n", v->name);
}

static bool parse_expander(char s, expander_type* type);

static bool parse_subscript(context* ctx,
                            subscript* ss, const char* spec);

static inline bool
variable_parse(context* ctx, variable* v, const char* s)
{
  mcsh_logger* logger = &ctx->entry->module->vm->logger;
  LOG(MCSH_LOG_DATA, MCSH_INFO, "parse: '%s'", s);
  // Defaults:
  v->type        = VARIABLE_PROTO;
  v->subscripted = false;

  if (strcmp(s, "@") == 0 ||
      strcmp(s, "#") == 0)
  {
    v->type = VARIABLE_SCALAR;
    strcpy(v->name, s);
    v->expander.type = EXPANDER_NONE;
    return true;
  }

  parse_expander(s[0], &v->expander.type);
  if (v->expander.type != EXPANDER_NONE) s++;

  size_t n = strlen(s);
  size_t start = 0;
  for (size_t i = start; i < n; i++)
  {
    if (s[i] == '.')
    {
      strlcpy(v->name, s+start, i+1);
      int m = n - i;
      v->subname = malloc(m);
      strcpy(v->subname, s+i+1);
      show("parsed: '%s' -> '%s'.'%s'",
           s, v->name, v->subname);
      v->type = VARIABLE_MODULE;
      return true;
    }
    if (s[i] == '[')
    {
      strlcpy(v->name, s+start, i+1);
      v->name[i] = '\0';
      v->subscripted = true;
      parse_subscript(ctx, &v->subscript, s+i);
      // Not always actually a scalar:
      v->type = VARIABLE_SCALAR;
    }
    else if (s[i] == '/')
    {
      strlcpy(v->name, s+start, i+1);
      v->name[i] = '\0';
      v->expander.type = EXPANDER_REGEX;
      // Start at character after slash:
      v->expander.text = strdup(s+i+1);
      v->type = VARIABLE_SCALAR;
      return true;
    }
  }
  if (v->type == VARIABLE_PROTO)
  {
    v->type = VARIABLE_SCALAR;
    strcpy(v->name, s);
  }
  return true;
}

static bool
parse_expander(char s, expander_type* type)
{
  expander_type result = EXPANDER_NONE;
  switch (s)
  {
    case '#':
      result = EXPANDER_SIZE;
      break;
    case '+':
      result = EXPANDER_TEST;
      break;
    case '@':
      // This is $@ if there is no name
      result = EXPANDER_VIEW;
  }
  *type = result;
  return true;
}

static bool parse_contig(context* ctx, subscript* ss,
                         const char* spec);

/**
   Parse subscript of form:
   [contig,contig,...]
   where a contig one of:
     single integer N
     colon-separated integer range M:N
     colon-separated strided integer range M:N:S (NYI)
     string key
   Commas are not yet implemented (NYI): just a single contig now
 */
static bool
parse_subscript(context* ctx, subscript* ss, const char* spec)
{
  mcsh_logger* logger = &ctx->entry->module->vm->logger;
  LOG(MCSH_LOG_EVAL, MCSH_INFO, "parse_subscript(): '%s'", spec);
  size_t n = strlen(spec);
  valgrind_assert(spec[  0] == '[');
  valgrind_assert(spec[n-1] == ']');
  // printf("parse_subscript(): n=%zi\n", n);
  char t[n-1];
  strlcpy(t, &spec[1], n-1);
  LOG(MCSH_LOG_EVAL, MCSH_INFO,"parse_subscript(): spec: '%s'", t);
  list_array_init(&ss->contigs, 1);
  parse_contig(ctx, ss, t);
  return true;
}

static bool parse_subscript1(context* ctx,
                             subscript* ss,
                             const char* t,
                             size_t n);
static bool parse_subscript2(context* ctx,
                             subscript* ss,
                             const char* t, size_t n);

static bool
parse_contig(context* ctx, subscript* ss, const char* spec)
{
  // True if this is the last contig:
  bool done = false;
  const char* p   = spec;
  // Pointer to end of string:
  const char* end = spec + strlen(spec);
  do
  {
    char* q = strchrnul(p, ',');
    if (q == end) done = true;
    const char* colon = strchr(p, ':');
    // Colon not found or colon in next contig: single value
    if (colon == NULL || colon > q)
    {
      parse_subscript1(ctx, ss, p, q-p);
    }
    else
    {
      parse_subscript2(ctx, ss, p, colon - p);
    }
    if (!done) p = q+1;
  } while (!done);
  return true;
}

/** Single-value subscript "[t]" of strlen n */
static bool
parse_subscript1(context* ctx,
                 subscript* ss,
                 const char* t,
                 size_t n)
{
  contig* c = calloc(1, sizeof(*c));
  list_array_add(&ss->contigs, c);
  mcsh_value* value;
  to_value(ctx, t, &value);
  if (is_integer(value->string, NULL))
  {
    c->type = CONTIG_INTEGERS;
    int64_t v;
    mcsh_value_integer(value, &v);
    c->start = v;
    c->start_set = 1;
  }
  else
  {
    c->type = CONTIG_STRINGS;
    c->key  = strndup(value->string, n);
  }
  return true;
}

static bool parse_subscript2_start(context* ctx,
                                   const char* t, contig* c);
static bool parse_subscript2_end(  context* ctx,
                                   const char* t, contig* c);

/**
   Double-value subscript "[t0:t1]" of strlen n
   Puts parsed subscript in ss
*/
static bool
parse_subscript2(context* ctx, subscript* ss,
                 const char* t, size_t n)
{
  contig* c = calloc(1, sizeof(*c));
  c->is_range = true;
  list_array_add(&ss->contigs, c);
  char* p = strchr(t, ':');

  if (p == t)  // E.g., ":5"
  {
    p++;
    parse_subscript2_end(ctx, p, c);
  }
  else if (p == t+n-1)  // E.g., "5:"
  {
    *p = '\0';
    parse_subscript2_start(ctx, t, c);
  }
  else  // E.g., "5:8"
  {
    *p = '\0';
    p++;
    parse_subscript2_start(ctx, t, c);
    parse_subscript2_end  (ctx, p, c);
  }
  return true;
}

static bool
parse_subscript2_start(context* ctx,
                       const char* t, contig* c)
{
  // printf("parse_subscript2_start\n");
  mcsh_value* value;
  to_value(ctx, t, &value);
  int64_t v;
  mcsh_value_integer(value, &v);
  c->start_set = true;
  c->start = v;
  // printf("parse_subscript2_start = %zi\n", c->start);
  return true;
}

static bool
parse_subscript2_end(context* ctx, const char* t, contig* c)
{
  // printf("parse_subscript2_end\n");
  mcsh_value* value;
  to_value(ctx, t, &value);
  int64_t v;
  mcsh_value_integer(value, &v);
  c->end_set = true;
  c->end = v;
  // printf("parse_subscript2_end   = %zi\n", c->end);
  return true;
}

static bool
variable_eval(UNUSED mcsh_entry* entry,
              variable* v, mcsh_value* value, mcsh_value** subvalue)
{
  mcsh_module* module;
  bool rc;
  switch (value->type)
  {
    case MCSH_VALUE_LIST:
      assert(false);
      break;
    case MCSH_VALUE_TABLE:
      assert(false);
      break;
    case MCSH_VALUE_MODULE:
      module = value->module;
      rc = strmap_search(&module->vars, v->subname, (void*) subvalue);
      CHECK(rc, "variable_eval: failed: module subname: '%s'",
            v->subname);
      break;
    default:
      assert(false);
  }
  return true;
}

static bool subscript_eval_string(context* ctx, variable* v,
                                  mcsh_value* value,
                                  mcsh_value** output);
static bool subscript_eval_list  (context* ctx, variable* v,
                                  mcsh_value* value,
                                  mcsh_value** output);
static bool subscript_eval_table(context* ctx,
                                 variable* v,
                                 mcsh_value* value,
                                 mcsh_value** output);

static bool
subscript_eval(context* ctx, variable* v,
               mcsh_value* value, mcsh_value** output)
{
  bool result;
  switch (value->type)
  {
    case MCSH_VALUE_STRING:
      result = subscript_eval_string(ctx, v, value, output);
      break;
    case MCSH_VALUE_LIST:
      result = subscript_eval_list(ctx, v, value, output);
      break;
    case MCSH_VALUE_TABLE:
      result = subscript_eval_table(ctx, v, value, output);
      break;
    default:
      CHECK_FAILED("WEIRD VALUE");
  }
  return result;
}

static bool subscript_eval_string_contig(contig* C,
                                         mcsh_value* value,
                                         buffer* B);

static bool
subscript_eval_string(context* ctx,
                      variable* v,
                      mcsh_value* value, mcsh_value** output)
{
  bool rc;
  size_t n = strlen(value->string);
  // printf("subscript_eval_string n=%zi\n", n);
  buffer B;
  buffer_init(&B, n);
  // printf("contigs: %zi\n", v->subscript.contigs.size);
  for (size_t i = 0; i < v->subscript.contigs.size; i++)
  {
    rc = subscript_eval_string_contig(v->subscript.contigs.data[i],
                                      value, &B);
    CHECK(rc, "subscript failed!");
  }
  *output =
    mcsh_value_new_string(mcsh_entry_vm(ctx->entry), B.data);
  buffer_finalize(&B);
  return true;
}

/**
   value: Type is MCSH_VALUE_STRING
*/
static bool
subscript_eval_string_contig(contig* C, mcsh_value* value,
                             buffer* B)
{
  if (! C->is_range)
  {
    valgrind_assert(C->start_set);
    char a = value->string[C->start];
    buffer_catc(B, a);
  }
  else
  {
    size_t start, end;
    start = C->start_set ? C->start : 0;
    end   = C->end_set   ? C->end   : strlen(value->string);
    if (start > end) start = end;
    size_t length = end - start - 1;
    buffer_catn(B, value->string + start, length);
    // This is a slice, so there is no NUL byte
    B->data[B->length] = '\0';
  }
  return true;
}

/**
   value: Type is MCSH_VALUE_LIST
*/
static bool
subscript_eval_list_contig(contig* C, mcsh_value* value,
                           mcsh_value* output);

static bool
subscript_eval_list_scalar(context* ctx,
                           variable* v,
                           mcsh_value* value,
                           mcsh_value** output,
                           contig* C);

static bool
subscript_eval_list_list(context* ctx,
                         variable* v,
                         mcsh_value* value,
                         mcsh_value** output);

static bool
subscript_eval_list(context* ctx,
                    variable* v,
                    mcsh_value* value,
                    mcsh_value** output)
{
  mcsh_logger* logger = &ctx->entry->module->vm->logger;

  // The return value is either a scalar or a list:
  bool scalar = false;
  contig* C;
  if (v->subscript.contigs.size == 1)
  {
    C = v->subscript.contigs.data[0];
    if (C->start_set && ! C->end_set)
      scalar = true;
  }
  LOG(MCSH_LOG_EVAL, MCSH_INFO,
      "subscript_eval_list: scalar=%b", scalar);

  if (scalar)
    subscript_eval_list_scalar(ctx, v, value, output, C);
  else
    subscript_eval_list_list(ctx, v, value, output);

  return true;
}

static bool
subscript_eval_list_scalar(UNUSED context* ctx,
                           UNUSED variable* v,
                           mcsh_value* value,
                           mcsh_value** output,
                           contig* C)
{
  *output = value->list->data[C->start];
  return true;
}

static bool
subscript_eval_list_list(context* ctx,
                         variable* v,
                         /// The list:
                         mcsh_value* value,
                         mcsh_value** output)
{
  mcsh_value* result =
    mcsh_value_new_list_sized(ctx->entry->module->vm,
                                    v->subscript.contigs.size);

  bool rc;
  for (size_t i = 0; i < v->subscript.contigs.size; i++)
  {
    rc = subscript_eval_list_contig(v->subscript.contigs.data[i],
                                    value, result);
    CHECK(rc, "subscript failed!");
  }

  *output = result;
  return true;
}

/**
   value: Type is MCSH_VALUE_LIST
*/
static bool
subscript_eval_list_contig(contig* C,
                           /// The input list:
                           mcsh_value* value,
                           /// The output item or list:
                           mcsh_value* output)
{
  if (! C->is_range)
  {
    valgrind_assert(C->start_set);
    mcsh_value* item = value->list->data[C->start];
    list_array_add(output->list, item);
  }
  else
  {
    size_t start, end;
    start = C->start_set ? C->start : 0;
    end   = C->end_set   ? C->end   : value->list->size;
    if (start > end) start = end;
    size_t length = end - start;
    for (size_t i = 0; i < length; i++)
    {
      mcsh_value* item = value->list->data[C->start+i];
      list_array_add(output->list, item);
    }
  }

  return true;
}

static bool eval_table_1(context* ctx, variable* v, struct table* T,
                         mcsh_value** result);
static bool eval_table_2(context* ctx, variable* v, struct table* T,
                         mcsh_value** result);

static bool
subscript_eval_table(context* ctx,
                     variable* v,
                     mcsh_value* value,
                     mcsh_value** output)
{
  valgrind_assert(value->type == MCSH_VALUE_TABLE);
  struct table* T = value->table;
  mcsh_value* result;
  if (v->subscript.contigs.size == 1)
  {
    printf("eval_table_1\n");
    variable_print(v);
    eval_table_1(ctx, v, T, &result);
  }
  else
  {
    eval_table_2(ctx, v, T, &result);
  }
  *output = result;
  return true;
}

static bool subscript_eval_expander(variable* v,
                                    contig* c,
                                    mcsh_value* found,
                                    mcsh_value** result,
                                    mcsh_status* status);

static bool eval_table_1(context* ctx, variable* v, struct table* T,
                         mcsh_value** output)
{
  contig* c = v->subscript.contigs.data[0];
  valgrind_assert(c->type == CONTIG_STRINGS);
  mcsh_value* result;
  table_search(T, c->key, (void**) &result);
  subscript_eval_expander(v, c, result, &result, ctx->status);
  *output = result;
  return true;
}

static bool eval_table_2(context* ctx, variable* v, struct table* T,
                         mcsh_value** output)
{
  size_t count = v->subscript.contigs.size;
  mcsh_value* result =
    mcsh_value_new_table(ctx->entry->module->vm, count);
  for (size_t i = 0; i < count; i++)
  {
    contig* c = v->subscript.contigs.data[i];
    valgrind_assert(c->type == CONTIG_STRINGS);
    // printf("subscript_eval_table() spec: '%s'\n", c->key);
    mcsh_value* found;
    table_search(T, c->key, (void**) &found);
    mcsh_value* item = NULL;
    subscript_eval_expander(v, c, found, &item, ctx->status);
    table_add(result->table, c->key, item);
  }
  *output = result;
  return true;
}

static bool
subscript_eval_expander(variable* v, contig* c, mcsh_value* found,
                        mcsh_value** result, mcsh_status* status)
{
  if (v->expander.type == EXPANDER_TEST)
  {
    *result = mcsh_value_new_int(found != NULL);
    v->expander.type = EXPANDER_NONE;
  }
  else
  {
    RAISE_IF(found == NULL, status, NULL, 0, "mcsh.undefined",
             "could not find key: '%s' in table: '%s'",
             c->key, v->name);
    *result = found;
  }
  return true;
}


static mcsh_value* expand_size(mcsh_logger* logger,
                               mcsh_value* value);
static mcsh_value* expand_view(mcsh_vm* vm, variable* v,
                               mcsh_value* value);
static mcsh_value* expand_regex(mcsh_vm* vm,
                                variable* v, mcsh_value* value);

static mcsh_value*
expand(mcsh_vm* vm, variable* v, mcsh_value* value)
{
  mcsh_value* result;
  switch (v->expander.type)
  {
    case EXPANDER_SIZE:
      result = expand_size(&vm->logger, value);
      break;
    case EXPANDER_VIEW:
      result = expand_view(vm, v, value);
      break;
    case EXPANDER_REGEX:
      result = expand_regex(vm, v, value);
      break;
    default:
      valgrind_assert(false);
  }
  return result;
}

static mcsh_value*
expand_size(mcsh_logger* logger, mcsh_value* value)
{
  char t[PATH_MAX];
  size_t n;
  switch (value->type)
  {
    case MCSH_VALUE_STRING:
      n = mcsh_to_string(logger, t, PATH_MAX, value);
      break;
    case MCSH_VALUE_INT:
    case MCSH_VALUE_FLOAT:
      n = mcsh_to_string(logger, t, PATH_MAX, value);
      break;
    case MCSH_VALUE_LIST:
      n = value->list->size;
      break;
    case MCSH_VALUE_TABLE:
      n = value->table->size;
      break;
    default:
      valgrind_assert(false);
      // unreachable
      // prevents GCC 11.3.0 warning on Cygwin
      n = 0;
  }
  return mcsh_value_new_int(n);
}

static mcsh_value* expand_view_table(mcsh_vm* vm,
                                     mcsh_value* value);

static mcsh_value*
expand_view(mcsh_vm* vm, UNUSED variable* v, mcsh_value* value)
{
  mcsh_value* result;
  switch (value->type)
  {
    case MCSH_VALUE_STRING:
      printf("HASH\n");
      result = NULL;
      exit(1);
      break;
    case MCSH_VALUE_TABLE:
      result = expand_view_table(vm, value);
      break;
    default:
      valgrind_fail_msg("NYI");
  }
  return result;
}

static mcsh_value*
expand_view_table(mcsh_vm* vm, mcsh_value* value)
{
  struct table* T = value->table;
  mcsh_value* result = mcsh_value_new_list_sized(vm, T->size);
  mcsh_value_grab(&vm->logger, result);
  list_array* L = result->list;
  TABLE_FOREACH(T, item)
  {
    mcsh_value* s = mcsh_value_new_string(vm, item->key);
    list_array_add(L, s);
  }
  return result;
}


static mcsh_value* expand_regex_test(mcsh_vm* vm,
                                        variable* v,
                                        mcsh_value* value);
static mcsh_value* expand_regex_replace(mcsh_vm* vm,
                                        variable* v,
                                        mcsh_value* value,
                                        char* p);

static mcsh_value*
expand_regex(mcsh_vm* vm, variable* v, mcsh_value* value)
{
  printf("expand_regex: text: '%s'\n", v->expander.text);
  char* p = strchr(v->expander.text, '/');
  mcsh_value* result;
  if (p == NULL)
    result = expand_regex_test(vm, v, value);
  else
    result = expand_regex_replace(vm, v, value, p);
  return result;
}

static mcsh_value* expand_regex_test(UNUSED mcsh_vm* vm,
                                     variable* v,
                                     mcsh_value* value)
{
  char* pattern = v->expander.text;
  printf("pattern: '%s'\n", pattern);
  printf("string: '%s'\n", value->string);
  regmatch_t match;
  regex_t regex;
  int rc;
  rc = regcomp(&regex, pattern, REG_NOSUB);
  valgrind_assert(rc == 0);
  rc = regexec(&regex, value->string, 1, &match, 0);
  int found;
  char t[1024];
  switch (rc)
  {
    case 0:
      found = 1;
      break;
    case REG_NOMATCH:
      found = 0;
      break;
    default:
      regerror(rc, &regex, t, 1024);
      printf("error: %s\n", t);
      found = 0;
      break;
  }
  regfree(&regex);
  return mcsh_value_new_int(found);
}

static mcsh_value* replace(mcsh_vm* vm, char* original,
                           regoff_t start, regoff_t end,
                           char* substitute);

static mcsh_value*
expand_regex_replace(mcsh_vm* vm, variable* v, mcsh_value* value,
                     char* p)
{
  *p = '\0';
  p++;
  char* pattern = v->expander.text;
  char* subst = p;
  printf("pattern: '%s' subst: '%s'\n", pattern, subst);

  regmatch_t match;
  regex_t regex;
  regcomp(&regex, pattern, 0);
  int rc = regexec(&regex, value->string, 1, &match, 0);
  int found = (rc == 0);
  mcsh_value* result;
  if (found)
  {
    result = replace(vm, value->string, match.rm_so, match.rm_eo,
                     subst);
  }
  else
    result = mcsh_value_new_string(vm, value->string);

  regfree(&regex);
  return result;
}

static mcsh_value*
replace(mcsh_vm* vm, char* original,
        regoff_t start, regoff_t end,
        char* substitute)
{
  size_t original_length   = strlen(original);
  size_t substitute_length = strlen(substitute);
  char t[original_length + substitute_length];
  char* p = &t[0];
  copy(  p, original,     start);
  append(p, substitute,   substitute_length+1);
  append(p, original+end, original_length);
  mcsh_value* result = mcsh_value_new_string(vm, t);
  return result;
}

bool
mcsh_data_env(mcsh_vm* vm, const char* name, mcsh_value** result)
{
  char* v = getenv(name);
  if (v == NULL)
    return false;
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_INFO,
           "found env: '%s'", name);
  *result = mcsh_value_new_string(vm, v);
  return true;
}

bool
mcsh_drop_env(mcsh_vm* vm, const char* name)
{
  char* v = getenv(name);
  if (v == NULL) return false;
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_INFO,
           "dropping env: '%s'", name);
  unsetenv(name);
  return true;
}

bool
mcsh_data_special(mcsh_vm* vm, const char* name, mcsh_value** result)
{
  void* tmp;
  if (! table_search(vm->data->specials, name, &tmp))
    return false;

  *result = tmp;
  mcsh_log(&vm->logger, MCSH_LOG_DATA, MCSH_INFO,
           "found special: '%s' -> %p\n", name, *result);

  return true;
}

void
mcsh_join(mcsh_vm* vm, mcsh_value* L, mcsh_value* delimiter,
          mcsh_value** result)
{
  // For the result:
  buffer B;
  buffer_init(&B, 128);

  assert(L        ->type == MCSH_VALUE_LIST);
  assert(delimiter->type == MCSH_VALUE_STRING);

  char* d = delimiter->string;

  mcsh_join_list_to_buffer(&vm->logger, L->list, d, &B);
  *result = mcsh_value_new_string(vm, B.data);
  buffer_finalize(&B);
}

void
mcsh_join_list_to_buffer(mcsh_logger* logger,
                         list_array* L, const char* delimiter,
                         buffer* result)
{
  LOG(MCSH_LOG_DATA, MCSH_INFO, "join_list: %zi", L->size);
  buffer t;
  buffer_init(&t, 64);
  buffer_cat(result, "[");
  for (size_t i = 0; i < L->size; i++)
  {
    buffer_reset(&t);
    mcsh_value_buffer(logger, L->data[i], &t);
    buffer_catb(result, &t);
    LOG(MCSH_LOG_DATA, MCSH_TRACE, "t: '%s'\n", t.data);
    LOG(MCSH_LOG_DATA, MCSH_TRACE, "R: '%s'\n", result->data);
    if (i < L->size-1 && delimiter != NULL)
      buffer_cat(result, delimiter);
    // buffer_show(result);
  }
  buffer_cat(result, "]");
  buffer_finalize(&t);
}

void
mcsh_join_table_to_buffer(mcsh_logger* logger,
                          struct table* T, const char* delimiter,
                          buffer* result)
{
  LOG(MCSH_LOG_DATA, MCSH_INFO, "join_table: %zi", T->size);
  buffer t;
  buffer_init(&t, 64);
  buffer_cat(result, "{");
  int i = 0;
  TABLE_FOREACH(T, item)
  {
    buffer_reset(&t);
    buffer_cat(result, item->key);
    buffer_cat(result, ":");
    mcsh_value_buffer(logger, item->data, &t);
    buffer_catb(result, &t);
    if (i < T->size-1 && delimiter != NULL)
      buffer_cat(result, delimiter);
    i++;
  }
  buffer_cat(result, "}");
  buffer_finalize(&t);
}

static void UNUSED
variable_type_to_string(variable_type t, char* output)
{
  switch (t)
  {
    case VARIABLE_PROTO:
      strcpy(output, "PROTO");
      break;
    case VARIABLE_SCALAR:
      strcpy(output, "SCALAR");
      break;
    case VARIABLE_LIST:
      strcpy(output, "LIST");
      break;
    case VARIABLE_TABLE:
      strcpy(output, "TABLE");
      break;
    case VARIABLE_MODULE:
      strcpy(output, "MODULE");
      break;
    default:
      printf("variable_type_to_string(): unknown type: %i\n", t);
      exit(1);
      break;
  }
}
