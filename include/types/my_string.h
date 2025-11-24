#ifndef MY_STRING_H
#define MY_STRING_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

/**
 * @brief Represent a custom string type with dynamic length.
 */
typedef struct {
    char* data;
    uint32_t length;
} string_t;


/**
 * @brief Initialize a string_t from a C string.
 * @param str C string to initialize from
 * @return string_t Initialized string_t instance
 */
string_t string_create(const char* str, ssize_t length);

/**
 * @brief Append two string_t instances.
 * @param str1 First string_t
 * @param str2 Second string_t
 * @return string_t New string_t containing the concatenation of str1 and str2
 */
string_t string_append(const string_t* str1, const string_t* str2);

/**
 * @brief Concatenate a string_t with a C string.
 * @param str1 string_t instance
 * @param str2 C string to concatenate
 * @return string_t New string_t containing the concatenation of str1 and str2
 */
string_t string_concat(const string_t* str1, const char* str2);

/**
 * @brief 
 */
string_t* string_copy(const string_t* src);

/**
 * @brief Free the memory allocated for a string_t.
 * @param str Pointer to string_t to free
 */
void string_free(string_t* str);

/**
 * @brief Free the memory allocated for a string_t on the heap.
 * @param str Pointer to string_t to free
 */
void string_free_heap(string_t* str);

/**
 * @brief Get the C string from a string_t.
 * @param str Pointer to string_t
 * @return const char* C string representation
 */
const char* string_get(const string_t* str);

#endif