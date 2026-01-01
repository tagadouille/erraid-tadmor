#ifndef TASK_CREATOR_H
#define TASK_CREATOR_H

#include "types/argument.h"
#include "types/timing.h"

/**
 * @brief Create the full directory structure for the new task
 * @param base_path deamon root path.
 * @param timing the timing when the task will be executed (or not).
 * @param args arguments of the task.
 * @return the id of the created task on success, -1 on failure.
 */
int64_t create_task_dir(const timing_t *timing, const arguments_t *args);

#endif