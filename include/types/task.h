#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "types/timing.h"
#include "types/time_exitcode.h"
#include "types/argument.h"

//! maybe modify that because that's already exist in cmd
typedef enum{
    SI,
    SQ,
    INVALID // On pourra ajouter d'autres types plus tard
} command_type_t;

typedef struct command command_t;

/**
 * @brief Represents the command associated with a task.
 */
struct command{
    command_type_t type;

    union{
        arguments_t simple;
        struct
        {
            uint32_t count;
            command_t **cmds; // Array of pointers to commands
        } composed;
    } args;
};

/**
 * @brief Represents a task that can be executed by the scheduler.
 */
typedef struct task {
    uint64_t id;
    command_t *cmd;
    timing_t* timing;
} task_t;

/**
 * @brief Allocate and initialize a new task.
 */
task_t *task_create(uint64_t id);

/**
 * @brief Display the task information.
 * @param task Pointer to the task_t structure to display.
 */
void task_display(task_t* task);

/**
 * @brief Display the command information.
 * @param cmd Pointer to the command_t structure to display.
 */
void command_display(command_t *cmd);

/**
 * @brief Creates a complex command structure.
 * @param command Pointer to the command_t structure to initialize.
 * @param type The type of the complex command.
 * @return Pointer to the initialized command_t structure if success, NULL otherwise.
 */
command_t* create_command(command_t* command, command_type_t type);
/**
 * @brief Adds a simple command to the given command structure.
 * @param command Pointer to the command_t structure to add the simple command to.
 * @param simple_args Pointer to the arguments_t structure representing the simple command.
 * @return Pointer to the updated command_t structure if success, NULL otherwise
 */
command_t* add_simple_command(command_t* command, const arguments_t* simple_args);

/**
 * @brief Adds a complex command to the given command structure
 * @param og_command Pointer to the command_t structure to add the complex command to.
 * @param command Pointer to the command_t structure representing the complex command to add.
 */
command_t* add_complex_command(command_t* og_command, command_t* command);

/**
 * @brief Fills the command structure based on the provided buffer.
 * @param buffer The buffer containing command data.
 * @param size The size of the buffer.
 * @param cmd Pointer to the command_t structure to fill.
 * @param type The type of to command
 * @return 0 if success, -1 otherwise.
 */
command_t* command_filler(char* buffer, unsigned int size, command_t* cmd, command_type_t type);

/**
 * @brief Helper to recursively free a command structure.
 *        Works for both SI and SQ commands.
 */
void command_free(command_t *cmd);

/**
 * @brief Free all memory inside a task.
 */
void task_destroy(task_t *task);

/**
 * @brief Executes the given command.
 * @param cmd Pointer to the command_t structure representing the command to execute.
 */
void command_execute(const command_t *cmd);

#endifa