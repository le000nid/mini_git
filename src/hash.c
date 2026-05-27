#include "hash.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

uint64_t fnv1a_hash_bytes(const unsigned char *data, size_t size)
{
    uint64_t hash = FNV_OFFSET_BASIS;
    size_t i;

    for (i = 0; i < size; ++i) {
        hash ^= (uint64_t)data[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

int hash_file(const char *path, uint64_t *hash)
{
    FILE *file;
    int ch;
    uint64_t result;

    if (path == NULL || hash == NULL) {
        return 1;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    result = FNV_OFFSET_BASIS;

    while ((ch = fgetc(file)) != EOF) {
        result ^= (uint64_t)(unsigned char)ch;
        result *= FNV_PRIME;
    }

    if (ferror(file)) {
        fprintf(stderr, "Error: cannot read file '%s'\n", path);
        fclose(file);
        return 1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    *hash = result;

    return 0;
}