#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

uint64_t fnv1a_hash_bytes(const unsigned char *data, size_t size);
int hash_file(const char *path, uint64_t *hash);

#endif