#include "argument.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <endian.h>

char *arguments_parse(const unsigned char *buffer, unsigned int size) {
    if (buffer == NULL || size < 4) {
        fprintf(stderr, "Invalid buffer\n");
        return NULL;
    }
    unsigned int offset = 0;

    // --- Read ARGC ---
    uint32_t argc_be;
    memcpy(&argc_be, buffer + offset, sizeof(argc_be));
    uint32_t argc = be32toh(argc_be);
    offset += sizeof(argc_be);

    if (argc < 1 || argc > 1000) {
        fprintf(stderr, "Invalid ARGC value: %u\n", argc);
        return NULL;
    }

    // Temporary array to store each string
    char **argv = calloc(argc, sizeof(char *));
    if (!argv) {
        perror("calloc");
        return NULL;
    }

    unsigned int total_length = 0;

    // --- Read each string ---
    for (uint32_t i = 0; i < argc; i++) {
        if (offset + 4 > size) {
            fprintf(stderr, "Buffer too small for string length\n");
            goto error;
        }

        uint32_t len_be;
        memcpy(&len_be, buffer + offset, sizeof(len_be));
        uint32_t len = be32toh(len_be);
        offset += sizeof(len_be);

        if (offset + len > size) {
            fprintf(stderr, "Buffer too small for string data\n");
            goto error;
        }

        argv[i] = malloc(len + 1);
        if (!argv[i]) goto error;

        memcpy(argv[i], buffer + offset, len);
        argv[i][len] = '\0';
        offset += len;

        total_length += len + 1; // +1 for space
    }

    // --- Build a single string ---
    char *joined = malloc(total_length + 1);
    if (!joined) goto error;

    joined[0] = '\0';
    for (uint32_t i = 0; i < argc; i++) {
        strcat(joined, argv[i]);
        if (i < argc - 1) strcat(joined, " ");
    }

    // --- Cleanup temporary array ---
    for (uint32_t i = 0; i < argc; i++) free(argv[i]);
    free(argv);

    return joined;

error:
    for (uint32_t i = 0; i < argc; i++)
        if (argv[i]) free(argv[i]);
    free(argv);
    return NULL;
}