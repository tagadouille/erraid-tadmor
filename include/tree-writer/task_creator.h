#ifndef TASK_CREATOR_H
#define TASK_CREATOR_H

#include "types/argument.h"
#include "types/timing.h"

/**
 * @brief Find the next id not already used.
 * @return new uint64_t id or 0 if no tasks.
 */
uint64_t find_next_id();

/**
 * @brief Write the timing file.
 * @param path path for the timing file.
 * @param t timing to put in the file.
 * @return 0 on success and -1 on failure.
 */
int write_timing_file(const char *path, const timing_t *t);

/**
 * @brief Create the full directory structure for the new task
 * @param base_path deamon root path.
 * @param timing the timing when the task will be executed (or not).
 * @param args arguments of the task.
 * @return the id of the created task on success, -1 on failure.
 */
int64_t create_task_dir(const timing_t *timing, const arguments_t *args);

#endif