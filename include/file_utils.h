#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Read an entire file into a newly allocated buffer.
 *
 * This function handles file opening, size calculation, reading,
 * and memory allocation. It is intended to replace all duplicated 
 * read-loops and realloc blocks in cmd-reader, times-reader, etc.
 *
 * @param path Path to the file to read.
 * @param size_out Pointer to a variable that will receive the file size.
 * @return Pointer to the buffer containing the file contents, or NULL on error.
 *         The caller must free() the returned pointer.
 */
char *file_read_all(const char *path, ssize_t *size_out);

/**
 * @brief Join base path and filename into a newly allocated string.
 *
 * Example:
 *     path_join("/home/user/task", "cmd") → "/home/user/task/cmd"
 *
 * @param base The directory path.
 * @param child The filename or subdirectory name.
 * @return Newly allocated path string, or NULL on error.
 */
char *path_join(const char *base, const char *child);

#endif