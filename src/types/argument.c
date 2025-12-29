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

/**
 * @brief Read one string_t from a binary buffer.
 *        Format: [uint32_t length_be][data]
 * @return newly allocated string_t* or NULL.
 */
static string_t *read_one_string(const char *buf, size_t size, size_t *offset)
{
    if (*offset + sizeof(uint32_t) > size)
        return NULL;

    uint32_t len_be;
    memcpy(&len_be, buf + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);

    uint32_t len = be32toh(len_be);

    if (len == 0) {
        // skip, or return NULL : but NULL is correct (means invalid argument)
        return NULL;
    }

    if (*offset + len > size)
        return NULL;

    string_t *s = malloc(sizeof(string_t));
    if (!s)
        return NULL;

    s->data = malloc(len + 1);
    if (!s->data)
    {
        free(s);
        return NULL;
    }

    memcpy(s->data, buf + *offset, len);
    s->data[len] = '\0';
    s->length = len;

    *offset += len;

    return s;
}

bool arguments_parse_struct(const string_t *buf, unsigned int size, arguments_t *args)
{
    // Test if the arguments are valid
    if (!buf || !buf->data || !args || size < (unsigned int)sizeof(uint32_t))
    {
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

    if (argc == 0)
    {
        dprintf(STDERR_FILENO, "Invalid ARGC: 0\n");
        return false;
    }

    // Initialize args structure
    args->argc = argc;
    args->command = NULL;
    args->argv = NULL;

    // Allocate argv array
    uint32_t n_args = (argc > 0) ? (argc - 1) : 0;
    if (n_args > 0)
    {
        args->argv = calloc(n_args, sizeof(string_t *));
        if (!args->argv)
        {
            perror("calloc");
            return false;
        }
    }

    /* --- Read command first --- */
    args->command = read_one_string(buf->data, size, &offset);
    if (!args->command)
    {
        dprintf(STDERR_FILENO, "Failed to read command string\n");
        goto error;
    }

    /* --- Read argv[i] --- */
    for (uint32_t i = 0; i < n_args; i++)
    {
        args->argv[i] = read_one_string(buf->data, size, &offset);
        if (!args->argv[i])
        {
            dprintf(STDERR_FILENO, "Failed to read argv[%u]\n", i);
            goto error;
        }
    }

    return true;

    error:
    arguments_free(args);
    return false;
}

arguments_t *arguments_parse(const char *buffer, unsigned int size)
{
    if (!buffer || size <= 0)
    {
        dprintf(STDERR_FILENO, "arguments_parse: invalid input\n");
        return NULL;
    }

    // Creation of arguments_t structure
    string_t buf = string_create_from_cstr(buffer, size);
    if (!buf.data)
    {
        return NULL;
    }

    arguments_t *args = malloc(sizeof(arguments_t));
    if (!args)
    {
        perror("malloc");
        string_free(&buf);
        return NULL;
    }

    if (!arguments_parse_struct(&buf, size, args))
    {
        return NULL;
    }
    return args;
}

arguments_t *copy_arguments(const arguments_t *src) {
    
    if (src == NULL) {
        dprintf(STDERR_FILENO, "copy_arguments: src == NULL\n");
        return NULL;
    }

    arguments_t *dst = calloc(1, sizeof(arguments_t));
    if (!dst) {
        perror("calloc copy_arguments");
        return NULL;
    }
    dst->argc = src->argc;

    // copy command
    if (src->command) {
        dst->command = string_copy(src->command);

        if (!dst->command) {
            arguments_free(dst);
            free(dst);
            return NULL;
        }
    } else {
        dprintf(STDERR_FILENO, "copy_arguments: warning src->command == NULL\n");
        dst->command = NULL;
    }

    // copy argv array (if any)
    uint32_t n_args = 0;
    if (src->argc > 0) {
        // determine how many argv entries you really have; here I assume src->argc includes command.
        n_args = (src->argc > 0) ? (src->argc - 1) : 0;
    }

    if (n_args > 0) {
        dst->argv = calloc(n_args, sizeof(string_t *));

        if (!dst->argv) {
            perror("calloc dst->argv");
            arguments_free(dst);
            free(dst);
            return NULL;
        }

        for (uint32_t i = 0; i < n_args; ++i) {

            if (src->argv && src->argv[i]) {
                dst->argv[i] = string_copy(src->argv[i]);

                if (!dst->argv[i]) {
                    dprintf(STDERR_FILENO, "copy_arguments: string_copy(argv[%u]) failed\n", i);
                    arguments_free(dst);
                    free(dst);
                    return NULL;
                }
            } else {
                dst->argv[i] = NULL;
            }
        }
    } else {
        dst->argv = NULL;
    }
    return dst;
}


void arguments_free(arguments_t *a) {
    if (!a) return;

    if (a->command) {
        if (a->command->data) { free(a->command->data); a->command->data = NULL; }
        free(a->command);
        a->command = NULL;
    }

    if (a->argv) {
        // argv length may be a->argc (or a->argc-1 depending on your convention)
        uint32_t n = a->argc;
        for (uint32_t i = 0; i < n; ++i) {
            if (a->argv[i]) {
                if (a->argv[i]->data) { free(a->argv[i]->data); a->argv[i]->data = NULL; }
                free(a->argv[i]);
                a->argv[i] = NULL;
            }
        }
        free(a->argv);
        a->argv = NULL;
    }
    a->argc = 0;
}

char **arguments_to_argv(const arguments_t *args)
{
    if (!args || !args->command)
        return NULL;

    // +2 : un pour la commande, un pour le NULL final
    size_t n = args->argc + 2;

    char **argv = calloc(n, sizeof(char *));
    if (!argv)
        return NULL;

    // argv[0] = commande
    argv[0] = strdup(string_get(args->command));

    // arguments supplémentaires
    for (uint32_t i = 0; i < args->argc - 1; i++) {
        argv[i + 1] = strdup(string_get(args->argv[i]));
    }

    // execvp() exige un NULL final
    argv[n - 1] = NULL;

    return argv;
}