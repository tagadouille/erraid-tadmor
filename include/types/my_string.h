#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * @brief Represent a custom string type with dynamic length.
 */
typedef struct {
    char* data;
    uint32_t length;
} my_string_t;


/**
 * @brief Initialize a my_string_t from a C string.
 * @param str C string to initialize from
 * @return my_string_t Initialized my_string_t instance
 */
my_string_t my_string_init(const char* str);

/**
 * @brief Free the memory allocated for a my_string_t.
 * @param str Pointer to my_string_t to free
 */
void my_string_free(my_string_t* str);



#endif