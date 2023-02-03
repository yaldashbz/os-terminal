#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>

#include "split.h"
#include "shell.h"


void put_process_in_foreground(pid_t pid, bool cont, struct termios *tmodes) {
    int status;
    /* Put the job into the foreground. */
    tcsetpgrp(shell_terminal, pid);
    if (tmodes)
        tcsetattr(shell_terminal, TCSADRAIN, tmodes);
    /* Send the job a continue signal, if necessary. */
    if (cont && kill(-pid, SIGCONT) < 0)
        perror("kill (SIGCONT)");
    /* Wait for the process to report. */
    waitpid(pid, &status, WUNTRACED);
    /* Put the shell back in the foreground. */
    tcsetpgrp(shell_terminal, shell_pgid);
    /* Restore the shell's terminal modes. */
    if (tmodes)
        tcgetattr(shell_terminal, tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/**
 * Put a job in the background.
 *
 *     pid
 *     cont -- Send the process group a SIGCONT signal to wake it up.
 *
 */
void put_process_in_background(pid_t pid, bool cont) {
    /* Send the job a continue signal, if necessary. */
    if (cont && kill(-pid, SIGCONT) < 0)
        perror("kill (SIGCONT)");
}