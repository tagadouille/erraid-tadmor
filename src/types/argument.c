#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/argument.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>

bool arguments_parse_struct(const char *buf, unsigned int size, arguments_t *args)
{
    // Ensure buffer is valid and large enough for ARGC
    if (!args || !buf || size < sizeof(uint32_t)) {
        dprintf(STDERR_FILENO, "Invalid buffer\n");
        return false;
    }
    unsigned int offset = 0; // Décalage dans le buffer

    // * Read ARGC
    uint32_t argc_be; // Number of arguments
    memcpy(&argc_be, buf, sizeof(uint32_t));

    // Convert from big-endian to host byte order
    uint32_t argc = be32toh(argc_be);
    offset += sizeof(uint32_t);

    if (argc < 1) {
        dprintf(STDERR_FILENO, "Invalid ARGC value: %u\n", argc);
        return false;
    }

    // Allocate array for argument strings
    char **argv = calloc(argc, sizeof(char *));
    if (!argv) {
        perror("calloc");
        return false;
    }

    // Fill array
    for (uint32_t i = 0; i < argc; i++) {
        // Ensure buffer has space for length field
        if (offset + sizeof(uint32_t) > size) {
            dprintf(STDERR_FILENO, "Buffer too small for argument length\n");
            goto error;
        }

        // Calcul length of the argument
        uint32_t len_be;
        memcpy(&len_be, buf + offset, sizeof(uint32_t));
        uint32_t len = be32toh(len_be);
        offset += sizeof(uint32_t);

        // Ensure buffer has enough data for the argument string
        if (offset + len > size) {
            dprintf(STDERR_FILENO, "Buffer too small for data\n");
            goto error;
        }

        // Allocate and copy argument string
        argv[i] = malloc(len + 1);
        if (!argv[i]) {
            perror("malloc");
            goto error;
        }

        memcpy(argv[i], buf + offset, len);
        argv[i][len] = '\0';
        offset += len;
    }
    // Success : fill structure
    args->argc = argc;
    args->argv = argv;
    return true;

    error:
    // Cleanup on failure/error
    for (uint32_t i = 0; i < argc; i++)
        if (argv[i]) free(argv[i]);
    free(argv);
    return false;
}

char *arguments_parse(const char *buffer, unsigned int size)
{
    arguments_t args;
    if (!arguments_parse_struct(buffer, size, &args)) {
        return NULL;
    }

    // Calculate total length for the resulting string
    unsigned int total_length = 1; // for '\0' 
    for (uint32_t i = 0; i < args.argc; i++) {
        total_length += strlen(args.argv[i]) + 1; // +1 for space
    }

    // Allocate result string
    char *result = malloc(total_length);
    if (!result) {
        perror("malloc");
        arguments_free(&args);
        return NULL;
    }

    // Build the result string
    result[0] = '\0'; // Initialize as empty string
    for (uint32_t i = 0; i < args.argc; i++) {
        strcat(result, args.argv[i]);
        if (i < args.argc - 1) {
            strcat(result, " "); // Add space between arguments
        }
    }

    // Free allocated arguments structure
    arguments_free(&args);

    return result;
}

void arguments_free(arguments_t *args)
{
    if (args == NULL)
    {
        return;
    }
    // Free each string
    for (uint32_t i = 0; i < args->argc; i++)
    {
        free(args->argv[i]);
    }
    // Free array of pointers
    free(args->argv);

    // Reset structure
    args->argc = 0;
    args->argv = NULL;
}