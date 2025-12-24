
#include "mcsh-parser.h"

void
mcsh_parse_output_set(mcsh_node* node)
{
  mcsh.parse_state.output = node;
  mcsh.parse_state.status = MCSH_PARSE_OK;
}
