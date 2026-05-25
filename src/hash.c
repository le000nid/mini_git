#include "hash.h"

uint64_t fnv1a_hash_bytes(const unsigned char *data, size_t size)
{
    uint64_t hash = 14695981039346656037ULL;
    size_t i;

    for (i = 0; i < size; ++i) {
        hash ^= (uint64_t)data[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}