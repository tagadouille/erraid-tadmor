#include "types/task.h"
#include <time.h>

/**
 * @brief run the task if the task is due
 * @param task the task to run
 * @param minute_now the minute when the task will be executed
 */
int run_task_if_due(task_t *task, time_t minute_now);

/** @brief Execute any command with given file descriptors
 * @param cmd the command to execute
 * @param outfd the file descriptor of the stdout file
 * @param errfd the file descriptor of the stderr file
 * @param timespath the path to the times-exitcodes file
 * @param minute_now the minute to execute the task
 * @param is_top_level 1 if we are on the top of the command tree, 0 if not
 * @return 0 on success, -1 on failure
 */
int execute_any_command_fd(const command_t *cmd, int outfd, int errfd, const char *timespath, 
    time_t minute_now, int is_top_level);


/**
 * @brief Append one record to times-exitcodes
 * @param path the path to the times-exitcodes file
 * @param exitcode the exitcode
 * @param timestamp the timestamp
 */
int append_times_exitcodes(const char *path, uint16_t exitcode, time_t timestamp);