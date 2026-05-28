#include "minigit.h"

#include <stdio.h>
#include <string.h>

static void print_usage(void)
{
    printf("Usage:\n");
    printf("  minigit init\n");
    printf("  minigit add <file>\n");
    printf("  minigit rm <file>\n");
    printf("  minigit commit <message>\n");
    printf("  minigit log\n");
    printf("  minigit files\n");
    printf("  minigit show <commit_id> <file>\n");
    printf("  minigit status\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        return minigit_init();
    }

    if (strcmp(argv[1], "add") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: add requires file path\n");
            return 1;
        }

        return minigit_add(argv[2]);
    }

    if (strcmp(argv[1], "rm") == 0) {
    if (argc != 3) {
        fprintf(stderr, "Error: rm requires file path\n");
        return 1;
    }

    return minigit_rm(argv[2]);
    }

    if (strcmp(argv[1], "commit") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: commit requires message\n");
            return 1;
        }

        return minigit_commit(argv[2]);
    }

    if (strcmp(argv[1], "log") == 0) {
        return minigit_log();
    }

    if (strcmp(argv[1], "files") == 0) {
        return minigit_files();
    }

    if (strcmp(argv[1], "show") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: show requires commit id and file path\n");
            return 1;
        }

        return minigit_show(argv[2], argv[3]);
    }

    if (strcmp(argv[1], "status") == 0) {
        return minigit_status();
    }

    fprintf(stderr, "Error: unknown command: %s\n", argv[1]);
    print_usage();

    return 1;
}