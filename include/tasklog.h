#ifndef TASKLOG_H
#define TASKLOG_H

#include <time.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Module for managing per-task log files (milestone 1).
 *
 * Before using any function, call tasklog_set_rundir() to define the
 * root directory of the tasks tree (e.g., "/tmp/$USER/erraid"). If this
 * function is not called, the module will fall back to using "./",
 * although calling tasklog_set_rundir is strongly recommended.
 *
 * All functions return 0 on success and -1 on failure (with errno set).
 */

// Sets the root directory of the tasks tree.
int tasklog_set_rundir(const char *run_dir);

// Adds an execution entry to tasks/<id>/executions log.
int log_add_execution(uint64_t taskid, time_t when, int exit_code);

// Replaces atomically tasks/<id>/stdout by the content buf[0..len-1].
int log_write_stdout( uint64_t taskid, const char *buf, size_t len);

// Replaces atomically tasks/<id>/stderr by the content buf[0..len-1].
int log_write_stderr( uint64_t taskid, const char *buf, size_t len);

#endif 
