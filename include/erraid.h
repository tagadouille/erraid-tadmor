#ifndef ERRAID_H
#define ERRAID_H

#define _POSIX_C_SOURCE 200809L

#include <stddef.h>


#ifndef SLEEP_INTERVAL
#define SLEEP_INTERVAL 60
#endif

#ifndef LOG_NAME
#define LOG_NAME "erraid.log"
#endif
#ifndef PIDFILE_NAME
#define PIDFILE_NAME "erraid.pid"
#endif

#include "types/task.h"

// The current task being processed
extern task_t* curr_task;
extern string_t curr_output;
extern time_array_t* curr_time;

/**
 * Set the run directory of the daemon.
 * Must be called before daemon_init() or erraid_init_foreground().
 * 
 * Example: erraid_set_rundir("/tmp/marc/erraid");
 *
 * Returns 0 on success, -1 on failure (errno is set).
 */
int erraid_set_rundir(const char *rundir);

/**
 * Get the current run directory into `out` (size: outlen).
 * Returns 0 on success, -1 on failure.
 */
int erraid_get_rundir(char *out, size_t outlen);

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
 * Cleanup at daemon termination:
 *   - close logs
 *   - remove pidfile
 */
void daemon_cleanup(void);

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
