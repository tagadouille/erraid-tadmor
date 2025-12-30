#define _GNU_SOURCE

#include "types/my_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t string_create(const void *data, ssize_t length){

    string_t s = {NULL, 0};

    if (!data || length <= 0)
        return s;

    s.data = malloc((size_t)length + 1);
    if (!s.data)
        return s;

    memcpy(s.data, data, (size_t)length);
    s.length = (uint32_t)length;

    return s;
}

string_t string_create_from_cstr(const char *str, ssize_t length)
{
    string_t s = {NULL, 0};

    if(!str) {
        return s;
    }

    if(length < 0) {
        length = (ssize_t)strlen(str);
    }

    s.data = malloc((size_t)length + 1);
    if (!s.data)
        return s;
    
    memcpy(s.data, str, (size_t)length);
    s.data[length] = '\0';
    s.length = (uint32_t)length;

    return s;
}

char *string_to_cstr(const string_t *str)
{
    if (!str || !str->data){
        return NULL;
    }

    char *out = malloc(str->length + 1);
    if (!out){
        return NULL;
    }

    memcpy(out, str->data, (size_t)str->length);
    out[str->length] = '\0';
    return out;
}

string_t string_append(const string_t* str1, const string_t* str2) {

    string_t result = {NULL, 0};

    if (!str1 || !str2)
        return result;

    result.data = malloc(str1->length + str2->length);
    if (!result.data)
        return result;

    memcpy(result.data, str1->data, str1->length);
    memcpy(result.data + str1->length, str2->data, str2->length);

    result.length = str1->length + str2->length;
    return result;
}

string_t string_concat(const string_t* str1, const void* data, ssize_t length) {

    string_t result = {NULL, 0};

    if (!str1 || !data || length <= 0)
        return result;

    result.data = malloc(str1->length + length);
    if (!result.data)
        return result;

    memcpy(result.data, str1->data, str1->length);
    memcpy(result.data + str1->length, data, (size_t)length);

    result.length = str1->length + (uint32_t)length;
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
        dst->data = NULL;
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
    return dst;
}

void string_free(string_t* src){
    if(src == NULL){
        return;
    }
    free(src -> data);
    src -> data = NULL;
    src -> length = 0;
    free(src);
}

const uint8_t* string_get(const string_t* str) {
    if (!str) return NULL;
    return str->data;
}
