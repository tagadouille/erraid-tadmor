#include "time_exitcode.h"
#include "int_types.h"

#include <stdlib.h>
#include <string.h>


time_exitcode_t* time_exitcode_new(void) {
    time_exitcode_t* record = (time_exitcode_t*)malloc(sizeof(time_exitcode_t));
    if (record == NULL) return NULL;
    
    record->time = 0;
    record->exitcode = 0;
    
    return record;
}

void free_time_exitcode(time_exitcode_t* record) {
    free(record);
}


time_exitcode_t* read_time_exitcode(int fd) {
    time_exitcode_t* record = time_exitcode_new();
    if (record == NULL) return NULL;
    
    // 1. Read TIME (uint64)
    if (read_uint64(fd, &record->time) == -1) {
        free_time_exitcode(record);
        return NULL;
    }

    // 2. Read EXITCODE (int32)
    if (read_int32(fd, &record->exitcode) == -1) {
        free_time_exitcode(record);
        return NULL;
    }

    return record;
}

int write_time_exitcode(int fd, const time_exitcode_t* record) {
    if (record == NULL) return -1;
    
    // 1. Write TIME (uint64)
    if (write_uint64(fd, record->time) == -1) {
        return -1;
    }

    // 2. Write EXITCODE (int32)
    if (write_int32(fd, record->exitcode) == -1) {
        return -1;
    }

    return 0;
}