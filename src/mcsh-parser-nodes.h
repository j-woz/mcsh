
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
  list_array_init(&node->children, 1);
  char* t = strndup(text, count);
  list_array_add(&node->children, t);
  node->line = line;
  // printf("mcsh_node_token: %p = '%s'\n", node, t);
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

