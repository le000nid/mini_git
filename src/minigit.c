#include "minigit.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MINIGIT_DIR ".minigit"
#define OBJECTS_DIR ".minigit/objects"
#define COMMITS_DIR ".minigit/commits"
#define HEAD_FILE ".minigit/HEAD"
#define INDEX_FILE ".minigit/index"

static int path_exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0;
}

static int create_directory(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    fprintf(stderr, "Error: cannot create directory '%s': %s\n",
            path,
            strerror(errno));

    return 1;
}

static int create_file_with_content(const char *path, const char *content)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot create file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    if (content != NULL) {
        if (fputs(content, file) == EOF) {
            fprintf(stderr, "Error: cannot write to file '%s'\n", path);
            fclose(file);
            return 1;
        }
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    return 0;
}

int minigit_init(void)
{
    if (path_exists(MINIGIT_DIR)) {
        printf("Mini-Git repository already exists in %s\n", MINIGIT_DIR);
        return 0;
    }

    if (create_directory(MINIGIT_DIR) != 0) {
        return 1;
    }

    if (create_directory(OBJECTS_DIR) != 0) {
        return 1;
    }

    if (create_directory(COMMITS_DIR) != 0) {
        return 1;
    }

    if (create_file_with_content(HEAD_FILE, "0\n") != 0) {
        return 1;
    }

    if (create_file_with_content(INDEX_FILE, "") != 0) {
        return 1;
    }

    printf("Initialized empty Mini-Git repository in %s\n", MINIGIT_DIR);

    return 0;
}

int minigit_add(const char *path)
{
    printf("minigit: add is not implemented yet: %s\n", path);
    return 0;
}

int minigit_commit(const char *message)
{
    printf("minigit: commit is not implemented yet: %s\n", message);
    return 0;
}

int minigit_log(void)
{
    printf("minigit: log is not implemented yet\n");
    return 0;
}

int minigit_files(void)
{
    printf("minigit: files is not implemented yet\n");
    return 0;
}

int minigit_show(const char *commit_id, const char *path)
{
    printf("minigit: show is not implemented yet: commit=%s file=%s\n",
           commit_id,
           path);
    return 0;
}

int minigit_status(void)
{
    printf("minigit: status is not implemented yet\n");
    return 0;
}