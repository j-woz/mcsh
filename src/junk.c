
// This PID activation conflicts with the $ builtin:

                mcsh_activation_pid,
static bool
activation_pid_get(UNUSED mcsh_entry* entry,
                   UNUSED const char* name,
                   mcsh_value** output,
                   mcsh_status* status)
{
  pid_t pid = getpid();
  mcsh_value* result = mcsh_value_construct_int(pid);
  *output = result;
  status->code = MCSH_OK;
  return true;
}

static bool
activation_pid_set(UNUSED mcsh_entry* entry,
                    UNUSED const char* name,
                    UNUSED mcsh_value* input,
                    mcsh_status* status)
{
  mcsh_raise(status, "MCSH_READ_ONLY", "cannot assign to this");
  return true;
}

  mcsh_activation_pid   .get = activation_pid_get;
  mcsh_activation_pid   .set = activation_pid_set;


  init("$",      &mcsh_activation_pid,    T, logger);


static inline void
thing_init(mcsh_thing* thing,
           mcsh_thing_type type,
           mcsh_thing* parent,
           mcsh_module* module)
{
  thing->type   = type;
  thing->parent = parent;
  thing->module = module;
}

void
mcsh_thing_init(mcsh_thing* thing,
                mcsh_thing_type type,
                mcsh_thing* parent,
                mcsh_module* module)
{
  thing_init(thing, type, parent, module);
}

mcsh_thing*
mcsh_thing_construct(mcsh_thing_type type,
                     mcsh_thing* parent,
                     mcsh_module* module)
{
  mcsh_thing* result = malloc(sizeof(mcsh_thing));
  thing_init(result, type, parent, module);
  return result;
}

mcsh_thing*
mcsh_thing_construct_token(mcsh_thing* parent,
                           mcsh_module* module,
                           char* s)
{
  mcsh_thing* result = malloc(sizeof(mcsh_thing));
  thing_init(result, MCSH_THING_TOKEN, parent, module);
  mcsh_token* token = malloc(sizeof(mcsh_token));
  token->text = strdup(s);
  result->data.token = token;
  return result;
}

//static inline mcsh_thing*
//mcsh_thing_construct_stmt(mcsh_stmt* stmt)
//{
//  mcsh_thing* thing = malloc_checked(sizeof(mcsh_thing));
//  thing->type = MCSH_THING_STMT;
//  thing->data.stmt = stmt;
//  thing->module = stmt->module;
//  thing->parent = stmt->parent;
//  return thing;
//}

#if 0
static list_array*
block_to_array(mcsh_block* block, list_array* P)
/** Convert block of stmts to list of string */
{
  list_array* stmts = &block->stmts.stmts;
  // printf("block2array\n");
  for (size_t i = 0; i < stmts->size; i++)
  {
    // printf("i=%zi\n", i);
    mcsh_stmt* stmt = stmts->data[i];
    // mcsh_stmt_print(stmt, 8);
    for (size_t j = 0; j < stmt->things.size; j++)
    {
      mcsh_thing* thing = stmt->things.data[j];
      char t[64];
      thing_str(thing, t);
      // printf("thing: %s\n", t);
      valgrind_assert(thing->type == MCSH_THING_TOKEN);
      char* name = thing->data.token->text;
      list_array_add(P, name);
    }
  }
  return P;
}
#endif
