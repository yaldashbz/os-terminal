#ifndef _SPLIT_H_
#define _SPLIT_H_

#include <stdio.h>
#include <string.h>

#define MAX_TOKS 100

struct token_desc {
    char **tokens_list;
    int tokens_num;
};
typedef struct token_desc token_desc_t;

token_desc_t *split_into_commands(char *input_line);

token_desc_t *split_into_params(char *command);

int is_direct_tok(char **t, char *R);

char **get_toks(char *line);

#endif