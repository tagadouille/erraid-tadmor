#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Reprensent type 'string'
 */
typedef struct 
{
    uint32_t length;
    char *data; // pointer 
} string_t;


/**
 * @brief Read string_t from a file descriptor.
 * @param fd File Descriptor
 * @return Pointer to a new string_t allocated, or NULL in case of error
 * Free with free_string()
 */
string_t* read_string(int fd);

/**
 * @brief Write string_t to a file descriptor.
 * @param fd File Descriptor
 * @param s string_t to write
 * @return 0 Success, -1 Error
 */
int write_string(int fd, const string_t* s);

/**
 * @brief Free the memory allocated for a string_t (buffer data and struct).
 * @param s string_t to free
 */
void free_string(string_t* s);

#endif