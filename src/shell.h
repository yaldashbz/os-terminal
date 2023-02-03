#ifndef _SHELL_H_
#define _SHELL_H_

#include <stdbool.h>
#include "sys/types.h"

#define MAX_COMMAND_LEN 512

extern bool shell_is_interactive;


int shell(int argc, char *argv[]);

#endif