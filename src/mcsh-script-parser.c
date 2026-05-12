
#include "mcsh-script-parser.h"
#include "mcsh-parser-nodes.h"

mcsh_node*
mcsh_script_token(char* term, int line)
{
  // Derive quoted-ness from the term itself rather than a global flag.
  // The lexer's unquoted STRING regex excludes '"', so a token is quoted
  // iff it starts with '"'. The old global flag was unreliable: bison's
  // one-token lookahead means the flag at reduction time may reflect
  // the next token rather than the one being reduced.
  size_t len = strlen(term);
  char* p;
  size_t count;
  bool quoted = (len >= 2 && term[0] == '"' && term[len-1] == '"');
  if (quoted)
  {
    p = &term[1];
    count = len - 2;
  }
  else
  {
    p = term;
    count = len;
  }

  mcsh_node* node = mcsh_node_token_sized(p, count, line);
  node->quoted = quoted;
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

mcsh_node*
mcsh_node_tag(char* left, mcsh_node* right, int line)
{
  mcsh_node* node = mcsh_node_construct(MCSH_NODE_TYPE_TAG, 2, line);
  mcsh_node* target = mcsh_script_token(left, line);

  list_array_add(&node->children, target);
  list_array_add(&node->children, right);

  return node;
}
