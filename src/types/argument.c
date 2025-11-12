#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/argument.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <endian.h>

char *arguments_parse(const char *buffer, unsigned int size) {
    if (buffer == NULL || size < 4) {
        dprintf(STDERR_FILENO, "Invalid buffer\n");
        return NULL;
    }
    unsigned int offset = 0;

    // --- Read ARGC ---
    uint32_t argc_be; //Number of arguments
    memcpy(&argc_be, buffer, sizeof(uint32_t));

    uint32_t argc = be32toh(argc_be);
    offset += sizeof(argc_be);

    if (argc < 1) {
        dprintf(STDERR_FILENO, "Invalid ARGC value: %u\n", argc);
        return NULL;
    }

    // Temporary array to store each string
    char **argv = calloc(argc, sizeof(char *));
    if (!argv) {
        perror("calloc");
        return NULL;
    }

    unsigned int total_length = 0;

    // --- Read each arguments ---
    for (uint32_t i = 0; i < argc; i++) {
        if (offset + sizeof(uint32_t) > size) {
            dprintf(STDERR_FILENO, "Buffer too small for argument length\n");
            goto error;
        }

        // Calcul length of the argument
        uint32_t len_be;
        memcpy(&len_be, buffer + offset, sizeof(uint32_t));
        uint32_t len = be32toh(len_be);
        offset += sizeof(uint32_t);

        if (offset + len > size) {
            dprintf(STDERR_FILENO, "Buffer too small for data\n");
            goto error;
        }

        argv[i] = malloc(len + 1);
        if (!argv[i]){
            perror("malloc");
            goto error;
        }

        memcpy(argv[i], buffer + offset, len);
        argv[i][len] = '\0';
        offset += len;

        total_length += len + 1; // +1 for space
    }

    // --- Build a single string ---
    char *joined = malloc(total_length + 1);
    if (!joined){
        perror("malloc");
        goto error;
    }

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

void arguments_free(arguments_t *args) {
    if (args == NULL) return;

    for (uint32_t i = 0; i < args->argc; i++) {
        free(args->argv[i]);
    }
    free(args->argv);
    args->argc = 0;
    args->argv = NULL;
}