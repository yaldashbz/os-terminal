#ifndef _SPLIT_H_
#define _SPLIT_H_

#include <stdio.h>
#include <string.h>

struct Splitted_tokens_desc {
    char **tokens_list;
    int tokens_num;
};
typedef Splitted_tokens_desc Token_desc;
Token_desc split_into_commands(char *input_line);
Token_desc split_into_params(char *command);
#endif