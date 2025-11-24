#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <stdint.h>
#include <stdbool.h>
#include "my_string.h"

typedef struct {
    uint32_t argc;   // Number of arguments
    string_t *command; // Command name
    string_t **argv;     // Array of argument strings
} arguments_t;

/**
 * @brief Parse into an arguments_t structure.
 * @param buffer Pointer to the buffer containing the serialized arguments.
 * @param size Size of the buffer.
 * @param args Pointer to the arguments_t structure to fill.
 * @return true on success, false on failure.
 */
bool arguments_parse_struct(const string_t *buffer, unsigned int size, arguments_t *args);

/**
 * @brief Parse a binary buffer containing serialized arguments
 * and return them as a human-readable string (with spaces).
 *
 * @param buffer Pointer to the buffer containing the serialized arguments.
 * @param size Size of the buffer.
 * @return A newly allocated string containing the parsed arguments.
 */
arguments_t *arguments_parse(const char *buffer, unsigned int size);

/**
 * @brief Create a deep copy of an arguments_t structure.
 * @param dst Pointer to the destination arguments_t structure.
 * @param src Pointer to the source arguments_t structure.
 * @return A newly allocated copy of the arguments_t structure if successful, NULL otherwise.
 */
arguments_t* copy_arguments(arguments_t* dst, const arguments_t* src);

/**
 * @brief Free the memory allocated for the arguments structure.
 * @param args Pointer to the arguments structure to free.
 */
void arguments_free(arguments_t *args);
/**
 * @brief Convert an arguments_t structure to a NULL-terminated argv array.
 * @param args Pointer to the arguments_t structure.
 */
char **arguments_to_argv(const arguments_t *args);


#endif