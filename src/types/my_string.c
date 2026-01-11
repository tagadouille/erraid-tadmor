#define _GNU_SOURCE

#include "types/my_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

string_t *string_create(const void *data, ssize_t length) {

    string_t *s = calloc(1, sizeof(string_t));
    if (!s) {
        perror("calloc string_create");
        return NULL;
    }

    if (!data) {
        return s;
    }

    if (length < 0) {
        length = (ssize_t)strlen((const char *)data);
    }

    if (length == 0) {
        return s;
    }

    s->data = malloc((size_t)length + 1); // Allocate space for null terminator
    if (!s->data) {
        perror("malloc string_create");
        free(s);
        return NULL;
    }
    
    memcpy(s->data, data, (size_t)length);
    s->data[length] = '\0'; // Add null terminator
    s->length = (uint32_t)length;

    return s;
}

char *string_to_cstr(const string_t *str)
{
    if (!str) {
        return NULL;
    }

    // Allocate memory for the string data + null terminator
    char *out = malloc(str->length + 1);
    if (!out) {
        perror("malloc string_to_cstr");
        return NULL;
    }

    // Copy data if it exists
    if (str->length > 0 && str->data) {
        memcpy(out, str->data, str->length);
    }
    out[str->length] = '\0';

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
    result->data = malloc(result->length + 1); // Allocate space for null terminator

    if (!result->data) {
        perror("malloc string_append");
        free(result);
        return NULL;
    }

    memcpy(result->data, str1->data, str1->length);
    memcpy(result->data + str1->length, str2->data, str2->length);
    result->data[result->length] = '\0'; // Add null terminator

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
    result->data = malloc(result->length + 1); // Allocate space for null terminator

    if (!result->data) {
        perror("malloc string_concat");
        free(result);
        return NULL;
    }

    memcpy(result->data, str1->data, str1->length);
    memcpy(result->data + str1->length, data, (size_t)length);
    result->data[result->length] = '\0'; // Add null terminator

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

    return string_create(src->data, src->length);
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