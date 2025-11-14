#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include "argument.h"

typedef enum
{
    SI,
    SQ // On pourra ajouter d'autres types plus tard
} command_type_t;

typedef struct
{
    command_type_t type;

    union // Sert à stocker plusieurs types de données au même endroit (mais seulement un à la fois)
    {
        arguments_t simple;
        struct
        {
            uint32_t count;
            struct command **cmds; // Array of pointers to commands
        } composed;
    } u;
} command_t;

/**
 * @brief Executes the given command.
 * @param cmd Pointer to the command_t structure representing the command to execute.
 */
void command_execute(const command_t *cmd);

/**
 * @brief Frees the memory allocated for a command_t structure.
 * @param cmd Pointer to the command_t structure to free.
 */
void command_free(command_t *cmd);

#endif