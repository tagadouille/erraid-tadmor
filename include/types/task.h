#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>

#include "types/timing.h"
#include "types/time_exitcode.h"
#include "types/command.h"

/**
 * @brief Represents a task that can be executed by the scheduler.
 */
typedef struct task {
    uint32_t id;
    command_t *cmd;
    timing_t timing;
    time_exitcode_t history;
} task_t;

/**
 * @brief Allocate and initialize a new task.
 */
task_t *task_create(uint32_t id);

/**
 * @brief Free all memory inside a task.
 */
void task_destroy(task_t *task);

#endif