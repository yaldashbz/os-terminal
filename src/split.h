#ifndef _SPLIT_H_
#define _SPLIT_H_

#include <stdio.h>
#include <string.h>

#define MAX_TOKS 100

struct TokensDescStruct {
    char **tokens_list;
    int tokens_num;
};
typedef struct TokensDescStruct TokenDesc;

TokenDesc *split_into_commands(char *input_line);

TokenDesc *split_into_params(char *command);

int is_direct_tok(char **t, char *R);

char **get_toks(char *line);

void free_toks(char **toks);

int toks_length(char **t);

#endif