#include "split.h"

int MAXCMDINLINE = 10;
int MAXPARAMNUM = 20;

char *ltrim(char *s){
    while(((char)(*s) == ' ') || ((char)(*s) == '\t')) s++;
    return s;
}

char *rtrim(char *s){
    char* back = s + strlen(s);
    while(((char)(*--back) == ' ') || ((char)(*--back) == '\t'));
    *(back+1) = '\0';
    return s;
}

char *trim(char *s){
    return rtrim(ltrim(s)); 
}

token_desc split_into_commands(char *input_line){
    char **commands_list = (char **)malloc(MAXCMDINLINE * sizeof(char *)); 
    int commands_num = 0;
    char *token = strtok(input_line, ";");
    while (token != NULL){
        if (strlen(trim(token)) > 0){
            token = trim(token);
            len = strlen(token) + 1;
            r = malloc(len);
            strncpy(r, token, len);
            commands_list[commands_num] = r;
            commands_num += 1;
        }
        token = strtok(NULL, ";");
    }
    Token_desc *tokens_t = (Token_desc *)malloc(sizeof(Token_desc));
    tokens_t->tokens_num = commands_num;
    tokens_t->tokens_list = commands_list;
    return tokens_t;
}

token_desc split_into_params(char *command){
    char **param_list = (char **)malloc(MAXPARAMNUM * sizeof(char *)); 
    int param_num = 0;
    char *token = strtok(command, " ");
    while (token != NULL){
        token = trim(token);
        len = strlen(token) + 1;
        r = malloc(len);
        strncpy(r, token, len);
        param_list[param_num] = r;
        param_num += 1;
        token = strtok(NULL, ";");
    }
    Token_desc *tokens_t = (Token_desc *)malloc(sizeof(Token_desc));
    tokens_t->tokens_num = param_num;
    tokens_t->tokens_list = param_list;
    return tokens_t;
}
