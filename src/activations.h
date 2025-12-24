
#pragma once

#include "mcsh.h"

extern mcsh_activation mcsh_activation_time;
extern mcsh_activation mcsh_activation_random;

void mcsh_activation_init(mcsh_logger* logger, struct table* T);
