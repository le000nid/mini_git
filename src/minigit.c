#include "minigit.h"

#include <stdio.h>

int minigit_init(void)
{
    return 0;
}

int minigit_add(const char *path)
{
    printf("path=%s\n", path);
    return 0;
}

int minigit_commit(const char *message)
{
    printf("message=%s\n", message);
    return 0;
}

int minigit_log(void)
{
    return 0;
}

int minigit_files(void)
{
    return 0;
}

int minigit_show(const char *commit_id, const char *path)
{
    printf("commit=%s file=%s\n", commit_id, path);
    return 0;
}

int minigit_status(void)
{
    return 0;
}