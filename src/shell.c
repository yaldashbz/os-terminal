#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>

#include "shell.h"
#include "split.h"
#include "io.h"

char *BATCH = "batch";
char *INTERACTIVE = "interactive";

int shell_help(char *arg[]);
int shell_cd(char *arg[]);
int shell_pwd(char *arg[]);
int shell_wait(char *arg[]);
int shell_quit(char *arg[]);

typedef int command_fun (char *arg[]);
typedef struct fun_desc {
    command_fun *fun;
    char *cmd;
    char *doc;
    int param_num;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {help_command, "?", "show the help menu"},
    {quit_command, "quit", "quit the command shell"},
    {cd_command, "cd", "go to a directory"},
    {pwd_command, "pwd", "get the working directory address"}
};

int quit_command(char *arg[]){
    printf("Quitting\n");
    exit(0);
    return 1;
}

int help_command(char *arg[]) {
    for (int i=0; i < (sizeof(cmd_table)/sizeof(fun_desc_t)); i++) {
        printf("%s: %s\n",cmd_table[i].cmd, cmd_table[i].doc);
    }
    return 1;
}

int cd_command(char *arg[]){
    char* new_dir = arg[0];
    chdir(new_dir);
    return 1;
}

int pwd_command(char *arg[]){
    char current_path[500];
    printf(getcwd(current_path, sizeof(current_path)));
    return 1;
}

int start_shell(FILE *input_file){
    char *input_line;
    while(1){
        input_line = read_line(input_file);
        Token_desc commands_t = split_into_commands(input_line);
    }
    return 1;
}

int shell (int argc, char *argv[]) {
    if (!strcmp(argv[1], BATCH)){
        if (argc > 3){
            fprintf(stderr, "More arguments than expected for running batch mode.\n");
            exit(EXIT_FAILURE);
        }else {
            batch_fp = fopen(argv[2], "r");
            if (batch_fp == NULL) {
                fprintf(stderr, "Batch file does not exist\n");
                exit(EXIT_FAILURE);
            }
        }
        start_shell(batch_fp)
    }
    else if(!strcmp(argv[1], INTERACTIVE)){
        if (argc > 2){
            fprintf(stderr, "More arguments than expected for running interactive mode.\n");
            exit(EXIT_FAILURE);
        }
        start_shell(stdin)
    }
    else{
        fprintf(stderr, "First argument is not valid. First argument can be either batch or interactive.\n");
        exit(EXIT_FAILURE);
    }
}