
#include <stdlib.h>
#include <string.h>
#include "internal.h"

char* strclone(const char* string)
{
    int len;
    char* clone;

    len = strlen(string) + 1;

    clone = (char*) malloc(sizeof(char) * len);
    strcpy(clone, string);

    return clone;
}

