#include "types/my_string.h"
#include <stdio.h>
#include <stdlib.h>

static char *alloc_copy(const char *src, size_t len){
    char *p = malloc(len + 1);

    if (p == NULL){
        perror("malloc");
        return NULL;
    }
    memcpy(p, src, len);
    p[len] = '\0';
    return p;
}

string_t string_create(const char *str, ssize_t length){

    string_t s = {NULL, 0};

    if (!str || length <= 0)
        return s;

    // Allouer length + 1 pour le '\0'
    s.data = malloc((size_t)length + 1);
    if (!s.data)
        return s;

    memcpy(s.data, str, (size_t)length);
    s.data[length] = '\0';

    s.length = (uint32_t)length;
    dprintf(STDERR_FILENO, "string_create: length=%d, data='%.*s'\n", (int)length, (int)length, str);
    return s;
}


string_t string_append(const string_t *str1, const string_t *str2)
{
    string_t result = {NULL, 0};

    if (!str1 || !str2)
    {
        return result;
    }

    size_t len1 = (size_t)str1->length;
    size_t len2 = (size_t)str2->length;

    result.data = malloc(len1 + len2 + 1);
    if (!result.data)
    {
        return result;
    }

    memcpy(result.data, str1->data, len1);
    memcpy(result.data + len1, str2->data, len2);
    result.data[len1 + len2] = '\0';
    result.length = (uint32_t)(len1 + len2);
    return result;
}

string_t string_concat(const string_t *str1, const char *str2)
{
    string_t result = {NULL, 0};

    if (!str1 || !str2)
        return result;

    size_t len1 = (size_t)str1->length;
    size_t len2 = strlen(str2);

    result.data = malloc(len1 + len2 + 1);
    if (!result.data)
    {
        return result;
    }

    memcpy(result.data, str1->data, len1);
    memcpy(result.data + len1, str2, len2);
    result.data[len1 + len2] = '\0';
    result.length = (uint32_t)(len1 + len2);
    return result;
}

string_t *string_copy(const string_t *src){

    if (src == NULL){
        dprintf(STDERR_FILENO, "The source string can't be null\n");
        return NULL;
    }

    if (src->data == NULL) {
        dprintf(STDERR_FILENO, "The source string data can't be null\n");
        return NULL;
    }

    string_t *dst = calloc(1, sizeof(string_t));

    if (dst == NULL){
        perror("calloc");
        return NULL;
    }
    
    dst->length = src->length;

    if (src->data == NULL) {
        dst->data = NULL;
        return dst;
    }

    dst->data = alloc_copy(src->data, src->length);
    if (dst->data == NULL) {
        dprintf(STDERR_FILENO, "Error calloc copy\n");
        free(dst);
        return NULL;
    }

    return dst;
}


void string_free(string_t *str)
{
    if (!str)
        return;
    free(str->data);
    str->data = NULL;
    str->length = 0;
}

void string_free_heap(string_t *str)
{
    if (!str)
        return;
    free(str->data);
    free(str);
}

const char* string_get(const string_t* str) {
    if (!str || !str->data) return "";
    return str->data;
}
