#include <malloc.h>
#include "split.h"

#define TOK_SEP " \n:"

int MAX_CMD_IN_LINE = 10;
int MAX_PARAM_NUM = 20;

char *ltrim(char *s) {
    while (((int) (*s) < 33) && ((int) (*s) > 0)) s++;
    return s;
}

char *rtrim(char *s) {
    char *back = s + strlen(s) - 1;
    while (((int) (*back) < 33) && ((int) (*back) > 0)){
        back -= 1;
    };
    *(back + 1) = '\0';
    return s;
}

/**
 * Trim the given string
 * @param s: given string
 * @return trimmed string
 */
char *trim(char *s) {
    return rtrim(ltrim(s));
}

token_desc_t *split_into_commands(char *input_line) {
    /* remove \n from commands (happens in last command) */
    if (input_line[strlen(input_line) - 1] == '\n')
        input_line[strlen(input_line) - 1] = '\0';

    char **commands_list = (char **) malloc(MAX_CMD_IN_LINE * sizeof(char *));
    int commands_num = 0;
    char *token = strtok(input_line, ";");
    while (token != NULL) {
        if (strlen(trim(token)) > 0) {
            token = trim(token);
            int len = strlen(token) + 1;
            char *r = malloc(len);
            strncpy(r, token, len);
            commands_list[commands_num] = r;
            commands_num += 1;
        }
        token = strtok(NULL, ";");
    }
    token_desc_t *tokens_t = (token_desc_t *) malloc(sizeof(token_desc_t));
    tokens_t->tokens_num = commands_num;
    tokens_t->tokens_list = commands_list;
    return tokens_t;
}

token_desc_t *split_into_params(char *command) {
    /* remove escape char from commands (happens in reading from file) */
    if ((int) command[strlen(command) - 1] == 13)
        command[strlen(command) - 1] = '\0';

    char **param_list = (char **) malloc(MAX_PARAM_NUM * sizeof(char *));
    int param_num = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        if (strlen(trim(token)) > 0){
            token = trim(token);
            int len = strlen(token) + 1;
            char *r = malloc(len);
            strncpy(r, token, len);
            param_list[param_num] = r;
            param_num += 1;
            token = strtok(NULL, " ");
        }
    }
    param_list[param_num] = NULL;
    token_desc_t *tokens_t = (token_desc_t *) malloc(sizeof(token_desc_t));
    tokens_t->tokens_num = param_num;
    tokens_t->tokens_list = param_list;
    return tokens_t;
}

/**
 * Locates the character R in the token array t.
 * R must be of length 1.
 * Returns the token that is an exact match of R.
 */
int is_direct_tok(char **t, char *R) {
    for (int i = 0; i < MAX_TOKS - 1 && t[i]; i++) {
        if (strncmp(t[i], R, 1) == 0)
            return i;
    }
    return 0;
}

char **get_toks(char *line) {
    int i;
    char *c;

    char **toks = malloc(MAX_TOKS * sizeof(char *));

    /** Intializes an empty token array */
    for (i = 0; i < MAX_TOKS; i++) toks[i] = NULL;

    /* Start tokenizer on one line */
    c = strtok(line, TOK_SEP);
    for (i = 0; c && i < MAX_TOKS; i++) {
        toks[i] = c;
        /* scan for next token */
        c = strtok(NULL, TOK_SEP);
    }
    return toks;
}
