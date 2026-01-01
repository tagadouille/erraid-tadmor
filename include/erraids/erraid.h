#ifndef ERRAID_H
#define ERRAID_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <limits.h> 

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef SLEEP_INTERVAL
#define SLEEP_INTERVAL 60
#endif

#ifndef LOG_NAME
#define LOG_NAME "erraid.log"
#endif

#include "types/task.h"

// The current task being processed
extern task_t* curr_task;
extern string_t* curr_output;
extern time_array_t* curr_time;

//The file descriptor of the log file
extern int g_log_fd;

extern char tasksdir[PATH_MAX];

extern char g_log_path[PATH_MAX];

extern char g_run_dir[PATH_MAX];

extern volatile int running;

/**
 * Initialize AND daemonize:
 *   - double fork()
 *   - setsid()
 *   - redirects stdin/stdout/stderr
 *   - creates PID file
 *   - installs signals
 *
 * Returns 0 on success, -1 on error.
 */
int daemon_init(void);

/**
 * Initialize in foreground mode (no daemonization).
 * Useful for debugging with `-f`.
 *
 * Returns 0 on success, -1 on error.
 */
int erraid_init_foreground(void);

/**
 * Main execution loop of the daemon.
 * Blocks until a termination signal arrives.
 */
void daemon_run(void);

/**
 * Write a timestamped log message.
 * This is safe to call from anywhere inside the daemon.
 *
 * Example:
 *    write_log_msg("Loaded %d tasks", count);
 *
 * Returns 0 on success, -1 on failure.
 */
int write_log_msg(const char *fmt, ...);

/*
 * Run a task (when scheduling will be added).
 * Will be implemented after task structures are stable.
 *
 * int run_task(const task_t *t);
 */

#endif /* ERRAID_H */
