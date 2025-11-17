#include "types/my_string.h"

string_t string_create(const char* str, ssize_t length) {
    string_t s;

    s.data = NULL;
    s.length = 0;

    // Handle NULL input
    if (!str || length <= 0) {
        return s;
    }

    s.data = malloc((size_t)length + 1); // +1 for null terminator
    if (!s.data)
    {
        return s; // Return empty string on allocation failure
    }

    memcpy(s.data, str, (size_t)length);
    s.data[length] = '\0'; // Null terminate the string
    s.length = (uint32_t)length;
    return s;
}

string_t string_append(const string_t* str1, const string_t* str2) {
    string_t result;
    result.data = NULL;
    result.length = 0;

    if (!str1 || !str2) return result;

    size_t len1 = (size_t)str1->length;
    size_t len2 = (size_t)str2->length;

    result.data = malloc(len1 + len2 + 1);
    if (!result.data) {
        return result;
    }

    if (len1) memcpy(result.data, str1->data, len1);
    if (len2) memcpy(result.data + len1, str2->data, len2);
    result.data[len1 + len2] = '\0';
    result.length = (uint32_t)(len1 + len2);
    return result;
}

string_t string_concat(const string_t* str1, const char* str2) {
    string_t result;
    result.data = NULL;
    result.length = 0;

    if (!str1 || !str2) return result;

    size_t len1 = (size_t)str1->length;
    size_t len2 = strlen(str2);

    result.data = malloc(len1 + len2 + 1);
    if (!result.data) {
        return result;
    }

    if (len1) memcpy(result.data, str1->data, len1);
    if (len2) memcpy(result.data + len1, str2, len2);
    result.data[len1 + len2] = '\0';
    result.length = (uint32_t)(len1 + len2);
    return result;
}

string_t* string_copy(const string_t* src){
    if (!src || !src->data) return NULL;

    string_t* dst = malloc(sizeof(string_t));
    if (!dst) return NULL;

    dst->length = src->length;
    dst->data = malloc(dst->length + 1); // +1 for null terminator
    if (!dst->data){
        free(dst);
        return NULL;
    }

    memcpy(dst->data, src->data, dst->length);
    dst->data[dst->length] = '\0';

    return dst;
}


void string_free(string_t* str) {
    if (!str) return;
    if (str->data) {
        free(str->data);
        str->data = NULL;
    }
    str->length = 0;
}

void string_free_heap(string_t* str) {
    if (!str) return;
    if (str->data) free(str->data);
    free(str);
}