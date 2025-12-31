#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "types/timing.h"
#include "types/time_exitcode.h"
#include "types/argument.h"
#include "types/command.h"

/**
 * @brief Represents a task that can be executed by the scheduler.
 */
typedef struct task {
    uint64_t id;
    command_t *cmd;
    timing_t* timing;
} task_t;

typedef struct{
    uint32_t nbtask;
    task_t* all_task; //Array of size of nbtask
} all_task_t;

/**
 * @brief Allocate and initialize a new task.
 */
task_t *task_create(uint64_t id);

/**
 * @brief return a pointer of a copy the task gived
 * @param og_task the task to copy
 * @return a pointer of a copy the task gived, NULL if failure
 */
task_t* task_copy(task_t* og_task);

/**
 * @brief Display the task information.
 * @param task Pointer to the task_t structure to display.
 */
void task_display(task_t* task);

/**
 * @brief Adds a simple command to the given command structure.
 * @param command Pointer to the command_t structure to add the simple command to.
 * @param simple_args Pointer to the arguments_t structure representing the simple command.
 * @return Pointer to the updated command_t structure if success, NULL otherwise
 */
command_t* add_simple_command(command_t* command, arguments_t* simple_args);

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
 * @brief Free all memory inside a task.
 */
void task_destroy(task_t *task);

#endif