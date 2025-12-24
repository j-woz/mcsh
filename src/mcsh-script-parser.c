
#include "mcsh-script-parser.h"
#include "mcsh-parser-nodes.h"

mcsh_node*
mcsh_script_token(char* term, int line)
{
  // printf("add_token(): %i '%s'\n", mcsh_script_token_quoted, term);
  char* p;
  size_t count;
  if (mcsh_script_token_quoted)
  {
    // Remove double-quotes
    p = &term[1];
    count = strlen(p) - 1;
  }
  else
  {
    // Use the whole thing
    p = term;
    count = strlen(p);
  }

  mcsh_node* node = mcsh_node_token_sized(p, count, line);
  return node;
}

mcsh_node*
mcsh_node_term(mcsh_node* left, mcsh_node* right, int line)
{
  // printf("mcsh_node_term(%p, %p)\n", left, right);
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_PAIR, 2, line);
  list_array_add(&node->children, left);
  list_array_add(&node->children, right);
  return node;
}

mcsh_node*
mcsh_node_stmt(mcsh_node* left, mcsh_node* right, int line)
{
  // printf("mcsh_node_stmt(%p, %p)\n", left, right);
  mcsh_node* node =
    mcsh_node_construct(MCSH_NODE_TYPE_STMTS, 2, line);
  list_array_add(&node->children, left);
  list_array_add(&node->children, right);
  return node;
}

mcsh_node*
mcsh_node_block(mcsh_node* stmts, int line)
{
  // printf("mcsh_node_block(%p)\n", stmts);
  mcsh_node* node =
    mcsh_node_construct(MCSH_NODE_TYPE_BLOCK, 1, line);
  list_array_add(&node->children, stmts);
  return node;
}

mcsh_node*
mcsh_node_subcmd(mcsh_node* stmts, int line)
{
  // printf("mcsh_node_subst(%p)\n", stmts);
  mcsh_node* node =
    mcsh_node_construct(MCSH_NODE_TYPE_SUBCMD, 1, line);
  list_array_add(&node->children, stmts);
  return node;
}

mcsh_node*
mcsh_node_subfun(mcsh_node* stmts, int line)
{
  // printf("mcsh_node_subst(%p)\n", stmts);
  mcsh_node* node =
    mcsh_node_construct(MCSH_NODE_TYPE_SUBFUN, 1, line);
  list_array_add(&node->children, stmts);
  return node;
}
