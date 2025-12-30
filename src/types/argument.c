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
    s->length = len;

    *offset += len;

    return s;
}

bool arguments_parse_struct(const char *buf, unsigned int size, arguments_t *args)
{
    if (!buf || !args || size < sizeof(uint32_t)) {
        dprintf(STDERR_FILENO, "[arguments_parse_struct] invalid input\n");
        return false;
    }

    memset(args, 0, sizeof(*args));
    size_t offset = 0;

    /* --- read argc --- */
    if (offset + sizeof(uint32_t) > size) {
        dprintf(STDERR_FILENO, "[arguments_parse_struct] buffer too small for argc\n");
        return false;
    }

    uint32_t argc_be;
    memcpy(&argc_be, buf + offset, sizeof(uint32_t));
    uint32_t argc = be32toh(argc_be);
    offset += sizeof(uint32_t);

    if (argc == 0) {
        dprintf(STDERR_FILENO, "[arguments_parse_struct] invalid argc=%u\n", argc);
        return false;
    }

    args->argc = argc;

    args->argv = calloc(argc, sizeof(string_t *));
    if (!args->argv) {
        perror("[arguments_parse_struct] calloc argv");
        goto error;
    }

    /* --- read argv[i] --- */
    for (uint32_t i = 0; i < argc; ++i) {

        args->argv[i] = read_one_string(buf, size, &offset);

        if (!args->argv[i]) {
            dprintf(STDERR_FILENO,"[arguments_parse_struct] failed to read argv[%u]\n", i);
            goto error;
        }
    }

    return true;

    error:
    arguments_free(args);
    memset(args, 0, sizeof(*args));
    return false;
}

arguments_t *arguments_parse(const char *buffer,
                             unsigned int size)
{
    if (!buffer || size < sizeof(uint32_t)) {
        dprintf(STDERR_FILENO,
                "[arguments_parse] invalid input\n");
        return NULL;
    }

    arguments_t *args = calloc(1, sizeof(arguments_t));
    if (!args) {
        perror("[arguments_parse] calloc arguments");
        return NULL;
    }

    if (!arguments_parse_struct(buffer, size, args)) {
        dprintf(STDERR_FILENO,
                "[arguments_parse] parse failed\n");
        free(args);
        return NULL;
    }

    return args;
}


arguments_t *copy_arguments(const arguments_t *src) {

    if (!src) {
        dprintf(STDERR_FILENO, "copy_arguments: src == NULL\n");
        return NULL;
    }

    arguments_t *dst = calloc(1, sizeof(arguments_t));

    if (!dst) {
        perror("calloc copy_arguments");
        return NULL;
    }
    dst->argc = src->argc;

    if (src->argc > 0) {
        dst->argv = calloc(src->argc, sizeof(string_t *));

        if (!dst->argv) {
            perror("calloc dst->argv");
            free(dst);
            return NULL;
        }

        for (uint32_t i = 0; i < src->argc; ++i) {

            if (src->argv[i]) {
                dst->argv[i] = string_copy(src->argv[i]);

                if (!dst->argv[i]) {
                    dprintf(STDERR_FILENO, "copy_arguments: string_copy(argv[%u]) failed\n", i);
                    arguments_free(dst);
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

    if (a->argv) {
        // argv length may be a->argc (or a->argc-1 depending on your convention)
        uint32_t n = a->argc;
        for (uint32_t i = 0; i < n; ++i) {
            if (a->argv[i]) {
                string_free(a->argv[i]);
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
    if(!args){
        dprintf(2, "The argument can't be null");
        return NULL;
    }
    if(args->argc == 0){
        dprintf(2, "The argc can't be null");
        return NULL;
    }
    if (!args->argv){
        dprintf(2, "The argv can't be null");
        return NULL;
    }

    // +1 for the final NULL
    char **argv = calloc(args->argc + 1, sizeof(char *));
    if (!argv){
        perror("calloc");
        return NULL;
    }

    for (uint32_t i = 0; i < args->argc; i++) {

        argv[i] = strndup(args->argv[i]->data, args->argv[i]->length);

        if (!argv[i]) {
            for (uint32_t j = 0; j < i; j++)
                free(argv[j]);
            free(argv);
            dprintf(2, "strndup failed");
            return NULL;
        }
    }

    argv[args->argc] = NULL;
    return argv;
}