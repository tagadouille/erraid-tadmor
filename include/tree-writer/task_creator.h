#ifndef TASK_CREATOR_H
#define TASK_CREATOR_H

#include "types/argument.h"
#include "types/timing.h"

/**
 * @brief Find the next id not already used.
 * @param base_path root path.
 * @return new uint64_t id or 0 if no tasks.
 */
uint64_t find_next_id(const char *base_path);

/**
 * @brief Write the timing file.
 * @param fd file descriptor.
 * @param t timing to put in the file.
 * @return 0 on success and -1 on failure.
 */
int write_timing_file(int fd, const timing_t *t);

/**
 * @brief Write a simple task command (SI) to a file descriptor.
 * @param fd file descriptor.
 * @param args arguments of the command.
 * @return 0 on success, -1 on failure.
 */
int write_command_simple(int fd, const arguments_t *args);



#endif