#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>


void put_process_in_foreground(pid_t pid, bool cont, struct termios *tmodes);

/**
 * Put a job in the background.
 *
 *     pid
 *     cont -- Send the process group a SIGCONT signal to wake it up.
 *
 */
void put_process_in_background(pid_t pid, bool cont);

#endif
