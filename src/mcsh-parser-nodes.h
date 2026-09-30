
/*
  MCSH PARSER NODES
  Parser node constructor functions
*/

#pragma once

UNUSED static mcsh_node*
mcsh_node_expr_join(mcsh_node* left, mcsh_node* right, int line)
{
  // printf("mcsh_node_expr children: %p %p\n", left, right);
  char tmp[128];
  mcsh_node_to_string(tmp, right);
  // printf("mcsh_node_expr string: %s\n", tmp);
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_PAIR, 2, line);
  // printf("mcsh_node_expr node: %p\n", node);
  list_array_add(&node->children, left);
  list_array_add(&node->children, right);
  return node;
}

static mcsh_node* mcsh_node_token_sized(char const* text,
                                        size_t count, int line);

UNUSED static mcsh_node*
mcsh_node_token(char const* text, int line)
{
  return mcsh_node_token_sized(text, strlen(text), line);
}

static mcsh_node*
mcsh_node_token_sized(char const* text, size_t count, int line)
{
  // printf("mcsh_node_token: '%s'\n", text);
  mcsh_node* node = malloc_checked(sizeof(*node));
  node->type = MCSH_NODE_TYPE_TOKEN;
  node->quoted = false;
  list_array_init(&node->children, 1);
  char* t = strndup(text, count);
  list_array_add(&node->children, t);
  node->line = line;
  // printf("mcsh_node_token: %p = '%s'\n", node, t);
  return node;
}

UNUSED static mcsh_node*
mcsh_expr_node_token(char const* text, int line)
{
  // Derive quoted-ness from the token text itself, same approach as
  // mcsh_script_token() (mcsh-script-parser.c): the expr lexer's
  // STRINGLITERAL rule includes the surrounding quotes in the matched
  // text, so a token is quoted iff it starts and ends with '"'.
  size_t len = strlen(text);
  bool quoted = (len >= 2 && text[0] == '"' && text[len-1] == '"');
  const char* p;
  size_t count;
  if (quoted)
  {
    p = &text[1];
    count = len - 2;
  }
  else
  {
    p = text;
    count = len;
  }

  mcsh_node* node = mcsh_node_token_sized(p, count, line);
  node->quoted = quoted;
  return node;
}

/** Start an empty argument-list accumulator for a function call. */
UNUSED static mcsh_node*
mcsh_node_args_new(int line)
{
  return mcsh_node_construct(MCSH_NODE_TYPE_CALL, 2, line);
}

/** Append one evaluated-argument node to an accumulator from
    mcsh_node_args_new(). */
UNUSED static mcsh_node*
mcsh_node_args_add(mcsh_node* args, mcsh_node* arg)
{
  list_array_add(&args->children, arg);
  return args;
}

/** Build a function-call node: children[0] is the function name, the
    remaining children are the argument nodes taken from `args` (an
    accumulator from mcsh_node_args_new()). */
UNUSED static mcsh_node*
mcsh_node_call(char const* name, mcsh_node* args, int line)
{
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_CALL, 2, line);
  list_array_add(&node->children, strdup(name));
  for (size_t i = 0; i < args->children.size; i++)
    list_array_add(&node->children, args->children.data[i]);
  return node;
}

UNUSED static mcsh_node*
mcsh_node_op(mcsh_operator op, mcsh_node* left, mcsh_node* right,
             int line)
{
  char t[4];
  op_to_string(t, op);
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_OP, 3, line);
  mcsh_operator* opp = malloc_checked(sizeof(*opp));
  *opp = op;
  list_array_add(&node->children, opp);
  list_array_add(&node->children, left);
  list_array_add(&node->children, right);

  return node;
}

UNUSED static mcsh_node*
mcsh_node_tern(mcsh_node* condition,
               mcsh_node* left, mcsh_node* right,
               int line)
{
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_OP, 4, line);
  mcsh_operator* opp = malloc_checked(sizeof(*opp));
  *opp = MCSH_OP_TERN;
  list_array_add(&node->children, opp);
  list_array_add(&node->children, condition);
  list_array_add(&node->children, left);
  list_array_add(&node->children, right);

  return node;
}
