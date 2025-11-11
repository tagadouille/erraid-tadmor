#ifndef TIMES_READER_H
#define TIMES_READER_H
#include <unistd.h>

/**
 * @brief Reads and interprets the timing or times-exitcodes information from a specified file path.
 * @param path The file path to read the timing or times-exitcodes information from.
 * @param interpreter A pointer to the interpreter function that processes the read data.
 * @return 0 on success, -1 on failure.
 */
int timing_reader(const char* path, int (*interpreter)(char*, ssize_t));

/**
 * @brief Interprets the timing data read from a file.
 * @param data The data read from the timing file.
 * @param path The file path of the timing file.
 * @param size The size of the data read.
 * @return 0 on success, -1 on failure.
 */
int timing_interpreter(char* data, const char* path, ssize_t size);

/**
 * @brief Interprets the times-exicodes data read from a file.
 * @param data The data read from the times-exicodes data file.
 * @param path The file path of the times-exicodes data file.
 * @param size The size of the data read.
 * @return 0 on success, -1 on failure.
 */
int times_exitcodes_interpreter(char* data, const char* path, ssize_t size);

#endif