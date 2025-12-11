#define _GNU_SOURCE

#include "types/my_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t string_create(const char *str, ssize_t length){

    string_t s = {NULL, 0};

    if (!str || length <= 0)
        return s;

    s.data = malloc((size_t)length + 1);
    if (!s.data)
        return s;

    memcpy(s.data, str, (size_t)length);
    s.data[length] = '\0';

    s.length = (uint32_t)length;

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

string_t *string_copy(const string_t *src) {

    if (src == NULL) {
        dprintf(STDERR_FILENO, "string_copy: src == NULL\n");
        return NULL;
    }
    if (src->length > 0 && src->data == NULL) {
        dprintf(STDERR_FILENO, "string_copy: src->length=%u but src->data == NULL\n", src->length);
        return NULL;
    }

    string_t *dst = calloc(1, sizeof(string_t));
    if (!dst) {
        perror("calloc string_copy");
        return NULL;
    }

    dst->length = src->length;
    if (dst->length == 0) {
        dst->data = strdup(""); // ever not-NULL
        if (!dst->data) { free(dst); return NULL; }
        return dst;
    }

    dst->data = malloc((size_t)dst->length + 1);
    if (!dst->data) {
        perror("malloc string_copy");
        free(dst);
        return NULL;
    }

    memcpy(dst->data, src->data, dst->length);
    dst->data[dst->length] = '\0';
    return dst;
}

void string_free(string_t* src){
    if(src == NULL){
        return;
    }
    free(src -> data);
    free(src);
}

const char* string_get(const string_t* str) {
    if (!str || !str->data) return "";
    return str->data;
}
