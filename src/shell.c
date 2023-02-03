#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <stdbool.h>

#include "shell.h"
#include "split.h"
#include "io.h"

bool shell_is_interactive;

int help_command(char *arg[]);

int cd_command(char *arg[]);

int pwd_command(char *arg[]);

int quit_command(char *arg[]);

char *find_file_from_path(char *filename, char *path_tokens[]);

int io_redirect(char *arg[]);

void path_resolve(char *arg[], char *path_tokens[]);

void undo_signal();

void ignore_signal();

typedef int command_fun(char *arg[]);

typedef struct fun_desc {
    command_fun *fun;
    char *cmd;
    char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
        {help_command, "?",    "show the help menu"},
        {quit_command, "quit", "quit the command shell"},
        {cd_command,   "cd",   "go to a directory"},
        {pwd_command,  "pwd",  "get the working directory address"},
};

int quit_command(char *arg[]) {
    printf("Quitting\n");
    exit(0);
}

int help_command(char *arg[]) {
    for (int i = 0; i < (sizeof(cmd_table) / sizeof(fun_desc_t)); i++) {
        printf("%s: %s\n", cmd_table[i].cmd, cmd_table[i].doc);
    }
    return 0;
}

int cd_command(char *arg[]) {
    if (chdir(arg[0]) == -1) {
        printf("%s : %s\n", arg[0], strerror(errno));
        return 1;
    }
    return 0;
}

int pwd_command(char *arg[]) {
    char *cwd = (char *) malloc(PATH_MAX + 1);
    getcwd(cwd, PATH_MAX);
    if (cwd != NULL) {
        printf("%s\n", cwd);
        free(cwd);
        return 0;
    }
    return 1;
}

int lookup(char cmd[]) {
    for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++) {
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
    }
    return -1;
}

int process_exec(char *command) {
    char *path = (char *) malloc(PATH_MAX + 1);
    strncpy(path, getenv("PATH"), PATH_MAX);
    char **path_tokens = get_toks(path);

    if (!shell_is_interactive) {
        printf("\033[;33m%s\33[0m\n", command);
    }

    TokenDesc *param_t = split_into_params(command);
    if (param_t->tokens_num > MAX_COMMAND_LEN) {
        fprintf(stderr, "Exceeded maximum command length!\n");
        exit(0);
    }
//
//    printf("params before lookup [0] = %s\n", param_t->tokens_list[0]);
//    printf("len command = %lu\n", strlen(param_t->tokens_list[0]));
//    printf("last char = %c**\n", param_t->tokens_list[0][strlen(param_t->tokens_list[0]) - 1]);

    int index = lookup(param_t->tokens_list[0]);

//    if (param_t->tokens_num != cmd_table[index].param_num + 1) {
//        fprintf(stderr, "Arguments after command are not compatible!\n");
//        exit(0);
//    }

//    printf("index = %d\n", index);

    if (index >= 0) {
        cmd_table[index].fun(&param_t->tokens_list[1]);
    } else {

//        for (int i =0; i<param_t->tokens_num; i++) {
//            printf("param [%d] = %s\n", i, param_t->tokens_list[i]);
//            printf("path token[%d] = %s\n", i, path_tokens[i]);
//        }

        if (io_redirect(param_t->tokens_list) < 0) {
            exit(0);
        }

        path_resolve(param_t->tokens_list, path_tokens);
        undo_signal();
//        printf("params [0] = %s\n", param_t->tokens_list[0]);

        if (execv(param_t->tokens_list[0], param_t->tokens_list) < 0) {
            fprintf(stderr, "%s : Command not found\n", param_t->tokens_list[0]);
            exit(0);
        }

    }
    return 0;
}

int start_process(char *command) {
    if ((command[0] == 'c' && command[1] == 'd') || strcmp(command, "quit") >= 0) {
        process_exec(command);
        return 0;
    }
    pid_t pid = fork();
    if (pid == 0) {
        process_exec(command);
        exit(0);
    } else if (pid < 0) {
        fprintf(stderr, "Could not fork for %s.\n", command);
        return 1;
    }
    return 0;
}

int start_shell(FILE *input_file) {
    char *input_line;
    char *cwd_buf = (char *) malloc(PATH_MAX + 1);

    while (1) {
        if (!shell_is_interactive && feof(input_file))
            break;

        ignore_signal();

        if (shell_is_interactive) {
            getcwd(cwd_buf, PATH_MAX);
            printf("\033[;34m%s\33[0m > ", cwd_buf);
        }
        input_line = read_line(input_file);
        TokenDesc *commands_t = split_into_commands(input_line);
        char **tokens = commands_t->tokens_list;
        for (int i = 0; i < commands_t->tokens_num; i++) {
            start_process(tokens[i]);
        }
        int status;
        while ((wait(&status)) > 0);
    }
    return 0;
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
//    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
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

int shell(int argc, char *argv[]) {
    if (argc == 1) {
        shell_is_interactive = 1;
        start_shell(stdin);
    } else if (argc == 2) {
        FILE *batch_fp = fopen(argv[1], "r");
        if (batch_fp == NULL) {
            fprintf(stderr, "Batch file does not exist\n");
            exit(EXIT_FAILURE);
        } else {
            shell_is_interactive = 0;
            start_shell(batch_fp);
        }
        fclose(batch_fp);
    } else {
        fprintf(stderr, "More arguments than expected for running batch mode.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

//
//void init_shell() {
//    /* Check if we are running interactively */
//    shell_terminal = STDIN_FILENO;
//    shell_is_interactive = isatty(shell_terminal);
//
//    if (shell_is_interactive) {
//        /* Force the shell into foreground */
//        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
//            kill(-shell_pgid, SIGTTIN);
//
//        ignore_signal();
//        /* Saves the shell's process id */
//        shell_pgid = getpid();
//
//        /* Put shell in its own process group */
//        if (setpgid(shell_pgid, shell_pgid) < 0) {
//            fprintf(stderr, "Couldn’t put the shell in its own process group");
//            exit(1);
//        }
//
//        /* Take control of the terminal */
//        tcsetpgrp(shell_terminal, shell_pgid);
//        tcgetattr(shell_terminal, &shell_tmodes);
//    }
//}
//
//int shell(int argc, char *argv[]) {
//    char *input_bytes;
//    char **tokens;
//    char **path_tokens;
//    int fundex = -1;
//    int tokens_length = 0;
//    char *path = (char *) malloc(PATH_MAX + 1);
//    char *cwd_buf = (char *) malloc(PATH_MAX + 1);
//
//    /* copy a new path var to use */
//    strncpy(path, getenv("PATH"), PATH_MAX);
//    path_tokens = get_toks(path);
//
//
//    init_shell();
//
//    if (shell_is_interactive) {
//        getcwd(cwd_buf, PATH_MAX);
//        printf("\033[;34m%s\33[0m > ", cwd_buf);
//    }
//
//    while ((input_bytes = read_line(stdin))) {
//        tokens = get_toks(input_bytes);
//        printf("%s, %s\n", tokens[0], tokens[1]);
//        tokens_length = toks_length(tokens);
//        int bg = 0;
//        if (tokens_length == 0) {
//            free_toks(tokens);
//            free(input_bytes);
//
//            if (shell_is_interactive) {
//                getcwd(cwd_buf, PATH_MAX);
//                /* Please only print shell prompts when standard input is not a tty */
//                printf("\033[;34m%s\33[0m > ", cwd_buf);
//            }
//            continue;
//        };
//        if (strcmp(tokens[tokens_length - 1], "&") == 0) {
//            bg = 1;
//            free(tokens[tokens_length - 1]);
//            tokens[tokens_length - 1] = NULL;
//        }
//        fundex = lookup(tokens[0]);
//        if (fundex >= 0) {
//            cmd_table[fundex].fun(&tokens[1]);
//        } else {
//            /* REPLACE this to run commands as programs. */
//            pid_t pid = fork();
//            if (pid == 0) {
//                if (io_redirect(tokens) < 0) {
//                    exit(0);
//                }
//                path_resolve(tokens, path_tokens);
//                undo_signal();
//
//                if (execv(tokens[0], tokens) < 0) {
//                    fprintf(stderr, "%s : Command not found\n", tokens[0]);
//                    exit(0);
//                }
//            } else {
//                if (!bg) {
//                    int child_status;
//                    if (waitpid(pid, &child_status, 0) < 0) {
//                        fprintf(stderr, "waitpid error\n");
//                    }
//                } else {
//                    printf("[%d] : %s\n", pid, input_bytes);
//                }
//            }
//        }
//        free_toks(tokens);
//        free(input_bytes);
//
//        if (shell_is_interactive) {
//            getcwd(cwd_buf, PATH_MAX);
//            /* Please only print shell prompts when standard input is not a tty */
//            printf("\033[;34m%s\33[0m > ", cwd_buf);
//        }
//    }
//    free(path);
//    free_toks(path_tokens);
//    free(cwd_buf);
//    return 0;
//}