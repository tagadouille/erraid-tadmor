#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "types/argument.h"
#include "types/my_string.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include <stdint.h>
#include <stdbool.h>

bool arguments_parse_struct(const string_t *buf, unsigned int size, arguments_t *args)
{
    // Test if the arguments are valid
    if (!buf || !buf->data || !args || size < (unsigned int) sizeof(uint32_t)) {
        dprintf(STDERR_FILENO, "arguments_parse_struct: invalid input\n");
        return false;
    }

    memset(args, 0, sizeof(*args));

    size_t offset = 0;

    // Read argc big-endian
    uint32_t argc_be;
    memcpy(&argc_be, buf->data + offset, sizeof(uint32_t));
    uint32_t argc = be32toh(argc_be);
    offset += sizeof(uint32_t);

    if (argc == 0) {
        dprintf(STDERR_FILENO, "Invalid ARGC: 0\n");
        return false;
    }

    // Initialize args structure
    args->argc = argc;
    args->command = NULL;
    args->argv = NULL;

    // Allocate argv array
    uint32_t n_args = (argc > 0) ? (argc - 1) : 0;
    if (n_args > 0) {
        args->argv = calloc(n_args, sizeof(string_t*));
        if (!args->argv) {
            perror("calloc");
            return false;
        }
    }

    // Read each argument
    for (uint32_t i = 0; i < argc; ++i) {

        // Read length big-endian
        if (offset + sizeof(uint32_t) > (size_t)size) {
            dprintf(STDERR_FILENO, "Buffer too small for length\n");
            goto error;
        }

        uint32_t len_be;
        memcpy(&len_be, buf->data + offset, sizeof(uint32_t));
        uint32_t len = be32toh(len_be);
        offset += sizeof(uint32_t);

        // * sanity: len should be reasonable 
        if (len > (uint32_t)size) {
            dprintf(STDERR_FILENO, "Bad length value\n"); 
            goto error;
        }

        if (offset + len > (size_t)size) {
            dprintf(STDERR_FILENO, "Buffer too small for data\n");
            goto error;
        }

        // Allocate structure and copy data
        string_t *s = malloc(sizeof(string_t));
        if (!s) {
            perror("malloc");
            goto error;
        }

        s->data = malloc((size_t)len + 1);
        if (!s->data) {
            free(s);
            perror("malloc");
            goto error;
        }

        memcpy(s->data, buf->data + offset, (size_t)len);
        s->data[len] = '\0';
        s->length = len;

        if (i == 0) {
            args->command = s;
        } else {
            args->argv[i - 1] = s;
        }

        offset += len;
    }

    return true;

error:
    arguments_free(args);
    return false;
}

arguments_t *arguments_parse(const char *buffer, unsigned int size){
    if (!buffer || size <= 0) {
        dprintf(STDERR_FILENO, "arguments_parse: invalid input\n");
        return NULL;
    }

    // Creation of arguments_t structure
    string_t buf = string_create(buffer, size);
    arguments_t* args = malloc(sizeof(arguments_t));
    if (!args) {
        perror("malloc");
        string_free(&buf);
        return NULL;
    }

    if (!arguments_parse_struct(&buf, size, args)) {
        string_free(&buf); 
        return NULL;
    }
    string_free(&buf);
    return args;
}


void arguments_free(arguments_t *args)
{
    if (!args){
        return;
    }

    if (args->command) {
        string_free_heap(args->command);
        args->command = NULL;
    }

    if (args->argv) {
        uint32_t n = (args->argc > 0 ? args->argc - 1 : 0);
        for (uint32_t i = 0; i < n; ++i) {
            if (args->argv[i]) {
                string_free_heap(args->argv[i]);
                args->argv[i] = NULL;
            }
        }
        free(args->argv);
        args->argv = NULL;
    }
    args->argc = 0;
}