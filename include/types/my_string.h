#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief Represent a custom string type with dynamic length.
 */
typedef struct string {
    uint8_t* data;
    uint32_t length;
} string_t;


/**
 * @brief Initialize a string_t from a buffer source.
 * @param data buffer source
 * @param length number of bytes
 * @return string_t* Initialized string_t instance
 */
string_t* string_create(const void* data, ssize_t length);

/**
 * @brief Get data as C string (NULL terminated copy).
 * @param str Pointer to string_t
 * @return char* Newly allocated C string (must be freed by caller) or NULL on failure
 */
char *string_to_cstr(const string_t* str);

/**
 * @brief Append two string_t instances.
 * @param str1 First string_t
 * @param str2 Second string_t
 * @return string_t* New string_t containing the concatenation of str1 and str2
 */
string_t* string_append(const string_t* str1, const string_t* str2);

/**
 * @brief Concatenate a string_t with a C string.
 * @param str1 string_t instance
 * @param data buffer source
 * @param length number of bytes
 * @return string_t* New string_t containing the concatenation of str1 and str2
 */
string_t* string_concat(const string_t* str1, const void* data, ssize_t length);

/**
 * @brief deep copy a string_t instance.
 * @param src Pointer to the source string_t
 */
string_t* string_copy(const string_t* src);

/**
 * @brief Free the memory allocated for a string_t.
 * @param src Pointer to string_t to free
 */
void string_free(string_t* src);

#endif