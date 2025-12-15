#include "types/task.h"
#include <time.h>

/**
 * @brief run the task if the task is due
 * @param task the task to run
 * @param minute_now the minute when the task will be executed
 */
int run_task_if_due(task_t *task, time_t minute_now);