
/**
   MCSH PARSER
*/

#pragma once

#include "mcsh.h"

void mcsh_parse_output_set(mcsh_node* node);

static void
mcsh_node_to_string(char* output, mcsh_node* node)
{
  if (node == NULL)
  {
    strcpy(output, "NULL");
    return;
  }
  switch (node->type)
  {
    case MCSH_NODE_TYPE_TOKEN:
      strcpy(output, "TOKEN");
      break;
    case MCSH_NODE_TYPE_OP:
      strcpy(output, "OP");
      break;
    case MCSH_NODE_TYPE_PAIR:
      strcpy(output, "PAIR");
      break;
    default:
      valgrind_fail();
  }
}

static mcsh_node*
mcsh_node_construct(mcsh_node_type type, size_t size, int line)
{
  mcsh_node* node = malloc_checked(sizeof(*node));
  node->type = type;
  list_array_init(&node->children, size);
  node->line = line;
  return node;
}

