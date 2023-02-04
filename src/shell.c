#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
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

/**
 * Lookup the given command in cmd_table
 * @param cmd: given command
 * @return index if found else -1
 */
int lookup(char cmd[]) {
    for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++) {
        if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
    }
    return -1;
}

/**
 * Execution of child process for commands (except cd & quit)
 * @param command: the given command
 * @return
 */
int process_exec(char *command) {
    char *path = (char *) malloc(PATH_MAX + 1);
    strncpy(path, getenv("PATH"), PATH_MAX);
    char **path_tokens = get_toks(path);

    if (!shell_is_interactive) {
        printf("\033[;33m%s\33[0m\n", command);
    }

    token_desc_t *param_t = split_into_params(command);
    char **tokens = param_t->tokens_list;
    if (strlen(command) > MAX_COMMAND_LEN) {
        fprintf(stderr, "Exceeded maximum command length!\n");
        exit(0);
    }
    int index = lookup(tokens[0]);

    if (index >= 0) {
        cmd_table[index].fun(&tokens[1]);
    } else {
        if (io_redirect(tokens) < 0) {
            exit(0);
        }
        path_resolve(tokens, path_tokens);
        undo_signal();

        if (execv(tokens[0], tokens) < 0) {
            fprintf(stderr, "%s : Command not found.\n", tokens[0]);
            exit(0);
        }
    }
    free(path);
    free(path_tokens);
    return 0;
}

int check_cd_quit(const char *command) {
    int is_cd = (command[0] == 'c' && command[1] == 'd');
    int is_quit = (command[0] == 'q' && command[1] == 'u' && command[2] == 'i' && command[3] == 't');
    return is_cd || is_quit;
}

pid_t start_process(char *command) {
    if (check_cd_quit(command)) {
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
    return pid;
}

/**
 * start the shell
 * @param input_file: can be stdin or batch file
 * @return
 */
int start_shell(FILE *input_file) {
    char *input_line;
    char *cwd_buf = (char *) malloc(PATH_MAX + 1);

    while (1) {
        if (!shell_is_interactive && feof(input_file)) break;
        ignore_signal();

        if (shell_is_interactive) {
            getcwd(cwd_buf, PATH_MAX);
            printf("\033[;34m%s\33[0m > ", cwd_buf);
        }
        input_line = read_line(input_file);
        if (input_line == NULL) {
            // CTRL + D
            if (shell_is_interactive) printf("\n");
            break;
        }
        token_desc_t *commands_t = split_into_commands(input_line);
        char **tokens = commands_t->tokens_list;
        for (int i = 0; i < commands_t->tokens_num; i++) {
            start_process(tokens[i]);
        }
        int status;
        while ((wait(&status)) > 0);
        free(input_line);
    }
    free(cwd_buf);
    return 0;
}


/**
 * Find executable file from PATH.
 * @param filename: file which should be found in path
 * @param path_tokens: PATH in env
 * @return
 */
char *find_file_from_path(char *filename, char *path_tokens[]) {
    char *ret = (char *) malloc(PATH_MAX + MAX_LINE + 2);
    struct dirent *ent;
    for (int i = 1; i < MAX_TOKS && path_tokens[i]; ++i) {
        DIR *dir;
        if ((dir = opendir(path_tokens[i])) == NULL)
            continue;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, filename) == 0) {
                strncpy(ret, path_tokens[i], PATH_MAX);
                strncat(ret, "/", 1);
                strncat(ret, filename, MAX_LINE);
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
 * @param arg: params of the given command
 * @param path_tokens: PATH in env
 */
void path_resolve(char *arg[], char *path_tokens[]) {
    char *eval = find_file_from_path(arg[0], path_tokens);
    if (eval != NULL) {
        arg[0] = eval;
    }
}

/**
 * Undo the default signal action for child job
 */
void undo_signal() {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

/**
 * Ignore interactive and job-control signals
 */
void ignore_signal() {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

/**
 * Input Redirection
 * @param arg: tokens of the command
 * @param in_redir: index or -1
 * @return 0 if can be redirected else -1
 */
int input_redirect(char *arg[], int in_redir) {
    int i, fd;
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
    return 0;
}

/**
 * Ouput Redirection
 * @param arg: tokens of the command
 * @param out_redir: index or -1
 * @return 0 if can be redirected else -1
 */
int output_redirect(char *arg[], int out_redir) {
    int i, fd;
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
    return 0;
}

/**
 *  IO Redirection
 * @param arg: tokens of the command
 * @return it can be redirected or not
 */
int io_redirect(char *arg[]) {
    int in_redir = is_direct_tok(arg, "<");
    int out_redir = is_direct_tok(arg, ">");
    if (in_redir != 0) {
        int check = input_redirect(arg, in_redir);
        if (check == -1) return -1;
    }
    if (out_redir != 0) {
        int check = output_redirect(arg, out_redir);
        if (check == -1) return -1;
    }
    return 0;
}

/**
 * The main function of the program
 * @param argc: argument count
 * @param argv: argument variables
 * @return
 */
int shell(int argc, char *argv[]) {
    if (argc == 1) {
        shell_is_interactive = 1;
        start_shell(stdin);
    } else if (argc == 2) {
        FILE *batch_fp = fopen(argv[1], "r");
        if (batch_fp == NULL) {
            fprintf(stderr, "Batch file does not exist.\n");
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
