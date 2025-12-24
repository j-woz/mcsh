
#pragma once

#include "list-array.h"

/** Returns the new string in fresh memory */
char* list_array_join_strings(list_array* L, char* delimiter);

/** Returns the new string in fresh memory */
char* list_array_join_values(list_array* L, char* delimiter);
