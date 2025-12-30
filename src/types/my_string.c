#define _GNU_SOURCE

#include "types/my_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t *string_create(const void *data, ssize_t length){

    string_t *s = calloc(1, sizeof(string_t));
    if (!s) {
        perror("calloc string_create");
        return NULL;
    }

    if(!data) {
        return s;
    }

    if(length < 0) {
        length = (ssize_t)strlen((const char *)data);
    }

    s->data = malloc((size_t)length);
    if (!s->data) {
        free(s);
        return NULL;
    }
    
    memcpy(s->data, data, (size_t)length);
    s->length = (uint32_t)length;

    return s;
}

char *string_to_cstr(const string_t *str)
{
    if (!str || !str->data){
        return NULL;
    }

    char *out = malloc(str->length);
    if (!out){
        return NULL;
    }

    memcpy(out, str->data, (size_t)str->length);
    return out;
}

string_t *string_append(const string_t* str1, const string_t* str2) {

    if (!str1 || !str2)
        return NULL;

    string_t *result = calloc(1, sizeof(string_t));
    if (!result) {
        perror("calloc string_append");
        return NULL;
    }

    result->length = str1->length + str2->length;
    result->data = malloc(result->length);

    if (!result->data) {
        perror("malloc string_append");
        free(result);
        return NULL;
    }

    memcpy(result->data, str1->data, str1->length);
    memcpy(result->data + str1->length, str2->data, str2->length);

    return result;
}

string_t *string_concat(const string_t* str1, const void* data, ssize_t length) {

    if (!str1 || !data || length < 0){
        dprintf(STDERR_FILENO, "string_concat: invalid arguments\n");
        return NULL;
    }

    string_t *result = calloc(1, sizeof(string_t));
    if (!result) {
        perror("calloc string_concat");
        return NULL;
    }

    result->length = str1->length + (uint32_t)length;
    result->data = malloc(result->length);

    if (!result->data) {
        perror("malloc string_concat");
        free(result);
        return NULL;
    }

    memcpy(result->data, str1->data, str1->length);
    memcpy(result->data + str1->length, data, (size_t)length);

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

    dst->data = malloc((size_t)dst->length);
    if (!dst->data) {
        perror("malloc string_copy");
        free(dst);
        return NULL;
    }

    memcpy(dst->data, src->data, dst->length);
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

const char* string_get(const string_t* str) {
    if (!str) return NULL;
    return string_to_cstr(str);
}