#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdint.h>
#include "types/my_string.h"
#include "communication/serialization/serialization.h"

#include <stdlib.h>
#include <stdio.h>

/* --------------------- string encoding ---------------------
   Wire format: LENGTH (uint32) followed by LENGTH bytes (no terminal 0).
   Here we assume string_t { uint32_t length; char *data; }.
-----------------------------------------------------------------*/

int encode_string(int fd, const string_t *s)
{
    if (!s){
        dprintf(2, "[encode_string] Error: The string can't be null\n");
        return -1;
    }

    if (s->length > LIMIT_MAX_STR_LEN){
        dprintf(2, "[encode_string] Error: The length of the string is too big\n");
        return -1;
    }
        
    if (encode_uint32(fd, s->length) < 0){
        dprintf(2, "[encode_string] Error : an error occured while encoding uint32\n");
        return -1;
    }

    dprintf(1, "Encoding this string %s of length %u\n", s->data, s->length);

    if (s->length > 0) {
        if (write_full(fd, s->data, s->length) < 0){
            dprintf(2, "[encode_string] Error : an error occured while writing the string into the pipe\n");
        return -1;
        }
    }

    return 0;
}

int decode_string(int fd, string_t *s)
{
    if (!s) {
        dprintf(STDERR_FILENO,
                "[decode_string] Error: string_t pointer is NULL\n");
        return -1;
    }

    /* Toujours repartir d’un état propre */
    if (s->data) {
        free(s->data);
        s->data = NULL;
    }
    s->length = 0;

    if (decode_uint32(fd, &s->length) < 0) {
        dprintf(STDERR_FILENO,
                "[decode_string] Error: can't decode uint32 length\n");
        return -1;
    }

    dprintf(STDERR_FILENO,
            "[decode_string] Decoded length = %u\n", s->length);

    if (s->length > LIMIT_MAX_STR_LEN) {
        dprintf(STDERR_FILENO,
                "[decode_string] Error: string length %u exceeds limit %u\n",
                s->length, LIMIT_MAX_STR_LEN);
        return -1;
    }

    if (s->length == 0) {
        dprintf(STDERR_FILENO,
                "[decode_string] Empty string\n");
        s->data = NULL;
        return 0;
    }

    s->data = malloc((size_t)s->length + 1);
    if (!s->data) {
        dprintf(STDERR_FILENO,
                "[decode_string] Error: malloc failed (%u bytes)\n",
                s->length + 1);
        s->length = 0;
        return -1;
    }

    if (read_full(fd, s->data, s->length) < 0) {
        dprintf(STDERR_FILENO,
                "[decode_string] Error: can't read %u bytes from fd\n",
                s->length);
        free(s->data);
        s->data = NULL;
        s->length = 0;
        return -1;
    }

    s->data[s->length] = '\0';

    dprintf(STDERR_FILENO,
            "[decode_string] String decoded: \"%s\"\n",
            s->data);

    return 0;
}

/* --------------------- timing encoding --------------------- */
/* timing: MINUTES(uint64), HOURS(uint32), DAYSOFWEEK(uint8) */

int encode_timing(int fd, const timing_t *t)
{
    if (!t)
        return -1;

    if (encode_uint64(fd, t->minutes) < 0)
        return -1;

    if (encode_uint32(fd, t->hours) < 0)
        return -1;

    if (encode_uint8(fd, t->daysofweek) < 0)
        return -1;

    return 0;
}

int decode_timing(int fd, timing_t *t)
{
    if (!t)
        return -1;

    if (decode_uint64(fd, &t->minutes) < 0)
        return -1;

    if (decode_uint32(fd, &t->hours) < 0)
        return -1;

    if (decode_uint8(fd, &t->daysofweek) < 0)
        return -1;

    return 0;
}

/* --------------------- arguments encoding ---------------------
 * Protocol: ARGC (uint32), then ARGV[0..ARGC-1] as strings.
 * Note: for CREATE request the COMMAND is exactly this arguments_t per protocol.
 * In your in-memory arguments_t we assume fields: uint32_t argc; string_t **argv;
 * (no separate command field). If your structure differs, adapt accordingly.
 -----------------------------------------------------------------*/

int encode_arguments(int fd, const arguments_t *args)
{
    if (!args){
        dprintf(2, "[encode_arguments] Error : the argument can't be NULL\n");
        return -1;
    }
        
    if (args->argc == 0 || args->argc > LIMIT_MAX_ARGC){
        dprintf(2, "[encode_arguments] Error : the LIMIT_MAX_ARGC has been reached\n");
        return -1;
    }

    if (encode_uint32(fd, args->argc) < 0){
        dprintf(2, "[encode_arguments] Error : an error occured while encoding uint32\n");
        return -1;
    }

    dprintf(1, "argc : %u\n", args->argc);

    for (uint32_t i = 0; i < args->argc; ++i) {

        if (!args->argv[i]) {
            dprintf(2, "[encode_arguments] WARNING: argv[%u] is NULL, encoding empty string\n", i);
            string_t empty = { .length = 1, .data = (uint8_t *)0 };

            if (encode_string(fd, &empty) < 0){
                dprintf(2, "[encode_arguments] Error : an error occured while encoding empty string\n");
                return -1;
            }    
            continue;
        }
        if (encode_string(fd, args->argv[i]) < 0) {
            dprintf(2, "[encode_arguments] Error: failed to encode string for argv[%u]\n", i);
            return -1;
        }
    }
    return 0;
}

int decode_arguments(int fd, arguments_t *args)
{
    if (!args){
        dprintf(2, "[decode_arguments] Error : the argument can't be NULL\n");
        return -1;
    }

    uint32_t argc;

    if (decode_uint32(fd, &argc) < 0){
        dprintf(2, "[decode_arguments] Error : an error occured while decoding uint32\n");
        return -1;
    }
        
    if (argc == 0 || argc > LIMIT_MAX_ARGC){
        dprintf(2, "[decode_arguments] Error : the LIMIT_MAX_ARGC has been reached\n");
        return -1;
    }

    args->argc = argc;
    args->argv = calloc((size_t)argc, sizeof(string_t *));

    if (!args->argv){
        perror("calloc");
        return -1;
    }
        
    for (uint32_t i = 0; i < argc; ++i) {
        args->argv[i] = malloc(sizeof(string_t));

        if (!args->argv[i]) {
            perror("malloc");
            arguments_free(args);
            return -1;
        }

        if (decode_string(fd, args->argv[i]) < 0) {
            dprintf(2, "[decode_arguments] Error : an error occured while decoding argv\n");
            arguments_free(args);
            return -1;
        }
    }

    return 0;
}

int encode_command(int fd, const command_t *cmd){

    if (!cmd) {
        dprintf(2, "[encode_command] ERROR: cmd is NULL\n");
        return -1;
    }

    /* --- type --- */
    if (encode_uint16(fd, (uint16_t)cmd->type) < 0) {
        dprintf(2, "[encode_command] ERROR: encode type failed\n");
        return -1;
    }

    dprintf(2, "[encode_command] type=%u\n", cmd->type);

    if(cmd->type == SI){
        if (!cmd->args.simple) {
            dprintf(2, "[encode_command] ERROR: simple args NULL\n");
            return -1;
        }
        return encode_arguments(fd, cmd->args.simple);
    }
    else{
        if (!cmd->args.composed.cmds) {
            dprintf(2, "[encode_command] ERROR: composed cmds NULL\n");
            return -1;
        }

        if (encode_uint32(fd, cmd->args.composed.count) < 0) {
            dprintf(2, "[encode_command] ERROR: encode count failed\n");
            return -1;
        }

        for (uint32_t i = 0; i < cmd->args.composed.count; ++i) {
            if (encode_command(fd, cmd->args.composed.cmds[i]) < 0) {
                dprintf(2,
                        "[encode_command] ERROR: encode sub-command %u\n", i);
                return -1;
            }
        }
        return 0;
    }
}

int decode_command(int fd, command_t **out){

    if (!out) {
        dprintf(2, "[decode_command] ERROR: out is NULL\n");
        return -1;
    }

    *out = NULL;

    command_t *cmd = calloc(1, sizeof(command_t));
    if (!cmd) {
        perror("[decode_command] calloc command");
        return -1;
    }

    /* --- type --- */
    uint16_t type;
    if (decode_uint16(fd, &type) < 0) {
        dprintf(2, "[decode_command] ERROR: decode_uint16(type) failed\n");
        goto error;
    }

    cmd->type = (command_type_t)type;

    dprintf(2, "[decode_command] type=%u\n", cmd->type);

    if (cmd->type == SI) {
        cmd->args.simple = calloc(1, sizeof(arguments_t));
        if (!cmd->args.simple) {
            free(cmd);
            return -1;
        }

        if (decode_arguments(fd, cmd->args.simple) < 0) {
            arguments_free(cmd->args.simple);
            free(cmd->args.simple);
            free(cmd);
            return -1;
        }
    }
    else{
        uint32_t count = 0;

        if (decode_uint32(fd, &count) < 0) {
            dprintf(2, "[decode_command] ERROR: decode_uint32(count) failed\n");
            goto error;
        }

        dprintf(2, "[decode_command] composed count=%u\n", count);

        if (count == 0) {
            dprintf(2, "[decode_command] ERROR: invalid composed count=%u\n", count);
            goto error;
        }

        cmd->args.composed.count = count;
        cmd->args.composed.cmds =
            calloc(count, sizeof(command_t *));
        if (!cmd->args.composed.cmds) {
            perror("[decode_command] calloc cmds");
            goto error;
        }

        for (uint32_t i = 0; i < count; ++i) {
            if (decode_command(fd, &cmd->args.composed.cmds[i]) < 0) {
                dprintf(2, "[decode_command] ERROR: decode sub-command %u\n", i);
                goto error;
            }
        }
    }

    *out = cmd;
    return 0;

    error:
    dprintf(2, "[decode_command] ERROR: cleanup after failure\n");
    command_free(cmd);
    *out = NULL;
    return -1;
}