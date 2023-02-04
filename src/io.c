#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "io.h"

char *read_line(FILE *fp) {
    char *line = malloc((MAX_LINE + 1) * sizeof(char));
    char *r = NULL;
    int len;
    char *s = fgets((char *) line, MAX_LINE, fp);
    if (!s) return s;
    len = strlen(s) + 1;
    r = malloc(len);
    strncpy(r, s, len);
    return r;
}