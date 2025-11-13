#include "types/my_string.h"

my_string_t my_string_create(const char* str) {
    my_string_t my_str;

    // Handle NULL input
    if (!str)
    {
       return (my_string_t){ .data = NULL, .length = 0 }; 
    }

    my_str.length = (uint32_t)strlen(str);
    my_str.data = (char*)malloc(my_str.length + 1); // +1 for null terminator
    
    if (my_str.data != NULL) { // Check for successful allocation
        memcpy(my_str.data, str, my_str.length + 1); // +1 to copy null terminator
    }
    return my_str;
}

void my_string_free(my_string_t* str) {
    if (str != NULL && str->data != NULL) { // Check for non-NULL pointer
        free(str->data);
        str->data = NULL;
        str->length = 0;
    }
}