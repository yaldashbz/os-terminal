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
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>

#include "shell.h"
#include "split.h"
#include "io.h"

char *BATCH = "batch";
char *INTERACTIVE = "interactive";

bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int help_command(char *arg[]);

int cd_command(char *arg[]);

int pwd_command(char *arg[]);

int quit_command(char *arg[]);

char *find_file_from_path(char *filename, char *path_tokens[]);

int io_redirect(char *arg[]);

void path_resolve(char *arg[], char *path_tokens[]);

void undo_signal();

typedef int command_fun(char *arg[]);

typedef struct fun_desc {
    command_fun *fun;
    char *cmd;
    char *doc;
    int param_num;
} fun_desc_t;

fun_desc_t cmd_table[] = {
        {help_command, "?",    "show the help menu"},
        {quit_command, "quit", "quit the command shell"},
        {cd_command,   "cd",   "go to a directory"},
        {pwd_command,  "pwd",  "get the working directory address"}
};

int quit_command(char *arg[]) {
    printf("Quitting\n");
    exit(0);
    return 1;
}

int help_command(char *arg[]) {
    for (int i = 0; i < (sizeof(cmd_table) / sizeof(fun_desc_t)); i++) {
        printf("%s: %s\n", cmd_table[i].cmd, cmd_table[i].doc);
    }
    return 1;
}

int cd_command(char *arg[]) {
    char *new_dir = arg[0];
    chdir(new_dir);
    return 1;
}

int pwd_command(char *arg[]) {
    char current_path[500];
    printf("%s\n", getcwd(current_path, sizeof(current_path)));
    return 1;
}

int lookup(char cmd[]) {
    for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++) {
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
    }
    return -1;
}

int start_shell(FILE *input_file){
    char *input_line;
    while(1){
        input_line = read_line(input_file);
        TokenDesc commands_t = split_into_commands(input_line);
        return 1;
    }
}

/**
 * Find execuatable file from PATH.
 */
char *find_file_from_path(char *filename, char *path_tokens[]) {
    char *ret = (char *) malloc(PATH_MAX + MAXLINE + 2);
    struct dirent *ent;
    for (int i = 1; i < MAX_TOKS && path_tokens[i]; ++i) {
        DIR *dir;
        if ((dir = opendir(path_tokens[i])) == NULL)
            continue;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, filename) == 0) {
                strncpy(ret, path_tokens[i], PATH_MAX);
                strncat(ret, "/", 1);
                strncat(ret, filename, MAXLINE);
                return ret;
            }
        }
        closedir(dir);
    }
    free(ret);
    return NULL;
}

/**
 * Path resolve
 */
void path_resolve(char *arg[], char *path_tokens[]) {
    char *eval = find_file_from_path(arg[0], path_tokens);
    if (eval != NULL) {
        arg[0] = eval;
    }
}

/**
 * undo the default singal action for child job
 */
void undo_signal() {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

/**
 *  Ignore interactive and job-control signals
*/
void ignore_signal() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    //signal(SIGCHLD, SIG_IGN);
}

/**
 * IO Redirection
 */
int io_redirect(char *arg[]) {
    int i, fd;
    int in_redir = is_direct_tok(arg, "<");
    int out_redir = is_direct_tok(arg, ">");
    if (in_redir != 0) {
        if (arg[in_redir + 1] == NULL ||
            strcmp(arg[in_redir + 1], ">") == 0 ||
            strcmp(arg[in_redir + 1], "<") == 0) {
            fprintf(stderr, "%s : Syntax error.\n", arg[0]);
            return -1;
        }
        if ((fd = open(arg[in_redir + 1], O_RDONLY, 0)) < 0) {
            fprintf(stderr, "%s : No such file or directory.\n", arg[in_redir + 1]);
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        for (i = in_redir; i < MAX_TOKS - 2 && arg[i + 2]; ++i) {
            free(arg[i]);
            arg[i] = arg[i + 2];
        }
        arg[i] = NULL;
    }
    if (out_redir != 0) {
        if (arg[out_redir + 1] == NULL ||
            strcmp(arg[out_redir + 1], ">") == 0 ||
            strcmp(arg[out_redir + 1], "<") == 0) {
            fprintf(stderr, "%s : Syntax error.\n", arg[0]);
            return -1;
        }
        if ((fd = open(arg[out_redir + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) <
            0) {
            fprintf(stderr, "%s : No such file or directory.\n", arg[out_redir + 1]);
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        for (i = out_redir; i < MAX_TOKS - 2 && arg[i + 2]; ++i) {
            free(arg[i]);
            arg[i] = arg[i + 2];
        }
        arg[i] = NULL;
    }
    return 0;
}

//
//int shell(int argc, char *argv[]) {
//    if (!strcmp(argv[1], BATCH)) {
//        if (argc > 3) {
//            fprintf(stderr, "More arguments than expected for running batch mode.\n");
//            exit(EXIT_FAILURE);
//        } else {
//            FILE *batch_fp = fopen(argv[2], "r");
//            if (batch_fp == NULL) {
//                fprintf(stderr, "Batch file does not exist\n");
//                exit(EXIT_FAILURE);
//            }
//            start_shell(batch_fp);
//        }
//    } else if (!strcmp(argv[1], INTERACTIVE)) {
//        if (argc > 2) {
//            fprintf(stderr, "More arguments than expected for running interactive mode.\n");
//            exit(EXIT_FAILURE);
//        }
//        start_shell(stdin);
//    } else {
//        fprintf(stderr, "First argument is not valid. First argument can be either batch or interactive.\n");
//        exit(EXIT_FAILURE);
//    }
//}

/**
 * Intialization procedures for this shell
 */
void init_shell() {
    /* Check if we are running interactively */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if(shell_is_interactive){
        /* Force the shell into foreground */
        while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        ignore_signal();
        /* Saves the shell's process id */
        shell_pgid = getpid();

        /* Put shell in its own process group */
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            fprintf(stderr, "Couldn’t put the shell in its own process group");
            exit(1);
        }

        /* Take control of the terminal */
        tcsetpgrp(shell_terminal, shell_pgid);
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

int shell(int argc, char *argv[]) {
    char *input_bytes;
    char* *tokens;
    char* *path_tokens;
    int fundex = -1;
    int tokens_length = 0;
    char* path = (char*) malloc(PATH_MAX + 1);
    char* cwd_buf = (char*) malloc(PATH_MAX + 1);

    /* copy a new path var to use */
    strncpy(path, getenv("PATH"), PATH_MAX);
    path_tokens = get_toks(path);


    init_shell();

    if (shell_is_interactive) {
        getcwd(cwd_buf, PATH_MAX);
        printf("\033[;34m%s\33[0m > ", cwd_buf);
    }

    while ((input_bytes = read_line(stdin))) {
        tokens = get_toks(input_bytes);
        printf("%s, %s\n", tokens[0], tokens[1]);
        tokens_length = toks_length(tokens);
        int bg = 0;
        if (tokens_length == 0) {
            free_toks(tokens);
            free(input_bytes);

            if (shell_is_interactive) {
                getcwd(cwd_buf, PATH_MAX);
                /* Please only print shell prompts when standard input is not a tty */
                printf("\033[;34m%s\33[0m > ", cwd_buf);
            }
            continue;
        };
        if (strcmp(tokens[tokens_length - 1], "&") == 0) {
            bg = 1;
            free(tokens[tokens_length - 1]);
            tokens[tokens_length - 1] = NULL;
        }
        fundex = lookup(tokens[0]);
        if (fundex >= 0) {
            cmd_table[fundex].fun(&tokens[1]);
        } else {
            /* REPLACE this to run commands as programs. */
            pid_t pid = fork();
            if (pid == 0) {
                if (io_redirect(tokens) < 0) {
                    exit(0);
                }
                path_resolve(tokens, path_tokens);
                undo_signal();

                if (execv(tokens[0], tokens) < 0) {
                    fprintf(stderr, "%s : Command not found\n", tokens[0]);
                    exit(0);
                }
            } else {
                if (!bg) {
                    int child_status;
                    if (waitpid(pid, &child_status, 0) < 0) {
                        fprintf(stderr, "waitpid error\n");
                    }
                } else {
                    printf("[%d] : %s\n", pid, input_bytes);
                }
            }
        }
        free_toks(tokens);
        free(input_bytes);

        if (shell_is_interactive) {
            getcwd(cwd_buf, PATH_MAX);
            /* Please only print shell prompts when standard input is not a tty */
            printf("\033[;34m%s\33[0m > ", cwd_buf);
        }
    }
    free(path);
    free_toks(path_tokens);
    free(cwd_buf);
    return 0;
}