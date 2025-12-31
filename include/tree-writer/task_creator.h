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
 */
int write_timing_file(int fd, const timing_t *t);


#endif