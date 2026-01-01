#define _GNU_SOURCE

#include "types/command.h"
#include <ctype.h>

command_t* create_command(command_type_t type){

    // Verification of type
    if (type != SI && type != SQ && type != IF && type != PL) {
        dprintf(STDERR_FILENO, "create_command: Unsupported command type %d\n", type);
        return NULL;
    }

    command_t* command = malloc(sizeof(command_t));
    if(command == NULL){
        perror("malloc");
        return NULL;
    }
    command->type = type;

    // Initialisation of the union fields
    // Simple CMD
    if(type == SI){
        command->args.simple = NULL;
    } 
    // Composed CMD
    else {
        command->args.composed.count = 0;
        command->args.composed.cmds = malloc(sizeof(command_t*) * 10); // Allocation initiale
        
        if(command->args.composed.cmds == NULL){
            perror("malloc");
            free(command);
            return NULL;
        }
    }
    return command;
}

void command_free(command_t *cmd){
    if (cmd == NULL)
        return;

    if (cmd->type == SI)
    {
        arguments_free(cmd->args.simple);
    }
    else if (cmd->type == SQ || cmd->type == IF || cmd->type == PL)
    {
        for (uint32_t i = 0; i < cmd->args.composed.count; i++)
        {
            command_free(cmd->args.composed.cmds[i]);
            cmd->args.composed.cmds[i] = NULL;
        }
        free(cmd->args.composed.cmds);
        cmd->args.composed.cmds = NULL;
    }

    cmd->args.composed.count = 0;
    free(cmd);
}

command_t* command_copy(const command_t* src) {

    if (src == NULL) {
        dprintf(STDERR_FILENO, "The command can't be null\n");
        return NULL;
    }

    command_t *cmd = calloc(1, sizeof(command_t));
    if (cmd == NULL) {
        perror("calloc");
        return NULL;
    }

    cmd->type = src->type;

    /* simple command */
    if (src->type == SI) {

        if(src->args.simple == NULL){
            dprintf(STDERR_FILENO, "The argument of the source task can't be null.\n");
            return NULL;
        }
        else{
            cmd->args.simple = copy_arguments(src->args.simple);

            if (cmd->args.simple == NULL) {
                dprintf(STDERR_FILENO, "Error copy arguments\n");
                command_free(cmd);
                return NULL;
            }
            if (cmd->args.simple->argv == NULL) {
                dprintf(STDERR_FILENO, "Error copy arguments\n");
                command_free(cmd);
                return NULL;
            }
        }
        
    } else {
        /* composed command */
        uint32_t count = src->args.composed.count;
        cmd->args.composed.count = count;

        if (count == 0) {
            cmd->args.composed.cmds = NULL;
        }
        else {
            cmd->args.composed.cmds = calloc(count, sizeof(command_t *));

            if (cmd->args.composed.cmds == NULL) {
                perror("calloc composed.cmds");
                command_free(cmd);
                return NULL;
            }

            for (uint32_t i = 0; i < count; ++i) {
                /* copy child recursively */
                cmd->args.composed.cmds[i] = command_copy(src->args.composed.cmds[i]);

                if (cmd->args.composed.cmds[i] == NULL) {
                    command_free(cmd);
                    return NULL;
                }
            }
        }
    }
    return cmd;
}

command_t* command_create_from_string(const char* str) {

    if (str == NULL || strlen(str) == 0) {
        dprintf(STDERR_FILENO, "command_create_from_string: input string is NULL or empty\n");
        return NULL;
    }

    // Create a mutable copy for strtok_r
    char *str_copy = strdup(str);
    if (str_copy == NULL) {
        perror("strdup");
        return NULL;
    }

    // Count the number of arguments
    uint32_t argc = 0;
    char *p = str_copy;

    while (*p) {
        // Skip leading spaces
        while (*p && isspace(*p)) p++;

        if (*p) {
            argc++;
            // Go to the end of the argument
            while (*p && !isspace(*p)) p++;
        }
    }
    free(str_copy);

    if (argc == 0) {
        dprintf(STDERR_FILENO, "command_create_from_string: no arguments found in string\n");
        return NULL;
    }

    // Allocate structures
    command_t *cmd = malloc(sizeof(command_t));
    if (cmd == NULL) {
        perror("malloc");
        return NULL;
    }
    cmd->type = SI;

    arguments_t *args = malloc(sizeof(arguments_t));
    if (args == NULL) {
        perror("malloc");
        free(cmd);
        return NULL;
    }
    args->argc = argc;
    args->argv = malloc(sizeof(string_t*) * argc);

    if (args->argv == NULL) {
        perror("malloc");
        free(args);
        free(cmd);
        return NULL;
    }

    cmd->args.simple = args;

    // 3. Tokenize and create string_t for each argument
    str_copy = strdup(str); // Another copy for the second tokenization
    if (str_copy == NULL) {
        perror("strdup");
        command_free(cmd);
        return NULL;
    }

    char *saveptr;
    char *token = strtok_r(str_copy, " ", &saveptr);
    uint32_t i = 0;

    while (token != NULL && i < argc) {

        if (strlen(token) > 0) { // Ignore empty tokens
            args->argv[i] = string_create(token, strlen(token));

            if (args->argv[i] == NULL) {
                dprintf(STDERR_FILENO, "string_create failed for argument %u\n", i);
                command_free(cmd);
                free(str_copy);
                return NULL;
            }
            i++;
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    free(str_copy);
    return cmd;
}

/** -------------- Display -------------------------- */

/**
 * @brief Display the arg of a command
 */
static void display_arg(string_t* string){

    if(string == NULL){
        dprintf(STDERR_FILENO, "[display_arg] The string can't be null");
        return;
    }

    if(string -> data == NULL){
        dprintf(STDERR_FILENO, "[display_arg] The data of the string can't be null");
        return;
    }

    char* data = string_to_cstr(string);

    if(data == NULL){
        dprintf(STDERR_FILENO, "string_to_cstr failed at display_arg");
        return;
    }

    for (size_t i = 0; i < string -> length; i++)
    {
        dprintf(STDOUT_FILENO, "%c", data[i]);
    }

    free(data);
}

void command_display(command_t *cmd){

    if (cmd == NULL){
        dprintf(STDERR_FILENO, "NULL command\n");
        return;
    }

    switch (cmd->type){
        case SI:
            if(cmd->args.simple == NULL){
                dprintf(STDERR_FILENO, "Error : The simple command can't be null\n");
                return;
            }

            dprintf(STDOUT_FILENO, " ");

            if(cmd->args.simple->argv != NULL){
                for (uint32_t i = 0; i < cmd->args.simple->argc; i++){
                    display_arg(cmd->args.simple->argv[i]);

                    if(i != cmd->args.simple->argc - 1){
                        dprintf(STDOUT_FILENO, " ");
                    }
                }
            }
            break;
        case SQ:
            dprintf(STDOUT_FILENO, "(");
            for (uint32_t i = 0; i < cmd->args.composed.count; i++){
                command_display(cmd->args.composed.cmds[i]);

                if(i != cmd->args.composed.count - 1){
                    dprintf(STDOUT_FILENO, "; ");
                }
            }
            dprintf(STDOUT_FILENO, ")");
            break;

        //TODO DISPLAY OF OTHER TYPES
        default:
            dprintf(STDERR_FILENO, "Unknown command type: %d\n", cmd->type);
            return;
    }
}