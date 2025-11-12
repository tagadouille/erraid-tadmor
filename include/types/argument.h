#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <stdint.h>

typedef struct {
    uint32_t argc;   // Number of arguments
    char **argv;     // Array of argument strings
} arguments_t;

/**
 * @brief Parse a binary buffer containing serialized arguments
 * and return them as a human-readable string (with spaces).
 *
 * @param buffer Pointer to the buffer containing the serialized arguments.
 * @param size Size of the buffer.
 * @return A newly allocated string containing the parsed arguments.
 */
char *arguments_parse(const char *buffer, unsigned int size);

/**
 * @brief Free the memory allocated for the arguments structure.
 * @param args Pointer to the arguments structure to free.
 */
void arguments_free(arguments_t *args);

#endif