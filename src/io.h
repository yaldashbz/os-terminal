#ifndef _io_H_
#define _io_H_

#include <stdio.h>
#define MAXLINE 1024
char *read_line(FILE *batch_file);
void free_line(char *ln);
#endif