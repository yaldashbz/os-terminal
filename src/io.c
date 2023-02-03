#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "io.h"

char *read_line(FILE *ifile) {
    char line[MAXLINE];
    char *r = NULL;
    int len;
    char *s = fgets((char *) line, MAXLINE, ifile);
    if (!s) return s;
    len = strlen(s) + 1;
    r = malloc(len);
    strncpy(r, s, len);
    return r;
}
