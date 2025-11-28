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
    string_t buf = string_create(buffer, size);
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
        string_free(&buf);
        // free(args);
        return NULL;
    }
    string_free(&buf);
    return args;
}

arguments_t *copy_arguments(arguments_t *dst, const arguments_t *src)
{
    // safety check
    if (!dst || !src)
    {
        return NULL;
    }

    dst->argc = src->argc;

    // Copy command
    if (src->command)
    {
        dst->command = string_copy(src->command);
        if (!dst->command)
        {
            perror("string_copy");
            return NULL;
        }
    }
    else
    {
        dst->command = NULL;
    }

    // Copy argv
    uint32_t n_args = (src->argc > 0 ? src->argc - 1 : 0);

    if (n_args > 0)
    {
        dst->argv = calloc(n_args, sizeof(string_t *));

        if (!dst->argv)
        {
            perror("calloc");
            string_free_heap(dst->command);
            return NULL;
        }

        for (uint32_t i = 0; i < n_args; i++)
        {
            if (src->argv[i])
            {
                dst->argv[i] = string_copy(src->argv[i]);
                if (!dst->argv[i])
                {
                    perror("string_copy");

                    // Free already copied elements
                    for (uint32_t j = 0; j < i; j++)
                        string_free_heap(dst->argv[j]);
                    free(dst->argv);
                    string_free_heap(dst->command);
                    return NULL;
                }
            }
            else
            {
                dst->argv[i] = NULL; // initialize NULL pointers
            }
        }
    }
    else
    {
        dst->argv = NULL;
    }
    return dst;
}

void arguments_free(arguments_t *args)
{
    if (!args)
    {
        return;
    }

    if (args->command)
    {
        string_free_heap(args->command);
        args->command = NULL;
    }

    if (args->argv)
    {
        //uint32_t n = (args->argc > 0 ? args->argc - 1 : 0);
        for (uint32_t i = 0; i < args->argc; ++i)
        {
            if (args->argv[i])
            {
                string_free_heap(args->argv[i]);
                args->argv[i] = NULL;
            }
        }
        free(args->argv);
        args->argv = NULL;
    }
    args->argc = 0;
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