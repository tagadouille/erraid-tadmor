#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types/argument.h"


/**
 * @brief Represents the type of a command.
 */
typedef enum{
    SI,
    SQ,
    PL,
    IF,
    INVALID
} command_type_t;

typedef struct command command_t;

/**
 * @brief Represents the command associated with a task.
 */
struct command{
    command_type_t type;

    union{
        arguments_t* simple;
        struct
        {
            uint32_t count;
            command_t **cmds; // Array of pointers to commands
        } composed;
    } args;
};

/**
 * @brief Creates a complex command structure.
 * @param type The type of the complex command.
 * @return Pointer to the initialized command_t structure if success, NULL otherwise.
 */
command_t* create_command(command_type_t type);

/**
 * @brief Create a command from a string representation.
 * @param str The string representation of the command.
 * @return Pointer to the created command_t structure, or NULL on failure.
 */
command_t* command_create_from_string(const char* str);

/**
 * @brief copy a command
 * @param src the command to be copied
 * @return a pointer to the copy, NULL if failure
 */
command_t* command_copy(const command_t* src);

/**
 * @brief Recursively free a command structure.
 * @param cmd Pointer to the command_t structure to free.
 */
void command_free(command_t *cmd);

/**
 * @brief Display the command information.
 * @param cmd Pointer to the command_t structure to display.
 */
void command_display(command_t *cmd);