#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "types/time_exitcode.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <endian.h>

time_array_t *time_exitcode_parse(const char *data, ssize_t size)
{
    const ssize_t REC_SIZE = sizeof(int64_t) + sizeof(uint16_t);

    // Buffer validation
    if (!data || size <= 0 || size % REC_SIZE != 0)
    {
        dprintf(STDERR_FILENO,
                "Invalid buffer size=%ld (not a multiple of %ld)\n",
                size, REC_SIZE);
        return NULL;
    }

    size_t nrecords = size / REC_SIZE;

    // Allocation of the principal structure
    time_array_t *arr = malloc(sizeof(time_array_t));
    if (!arr)
        return NULL;

    arr->nbruns = nrecords;
    arr->all_timecode = calloc(nrecords, sizeof(time_exitcode_t));
    
    if (!arr->all_timecode){
        free(arr);
        return NULL;
    }

    size_t offset = 0;

    for (size_t i = 0; i < nrecords; i++)
    {
        // ---- timestamp ----
        int64_t t_be;
        memcpy(&t_be, data + offset, sizeof(int64_t));
        offset += sizeof(int64_t);

        arr->all_timecode[i].time = (int64_t)be64toh(t_be);

        // ---- exitcode ----
        uint16_t c_be;
        memcpy(&c_be, data + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        arr->all_timecode[i].exitcode = (uint16_t)be16toh(c_be);
    }

    return arr;
}

void time_exitcode_show(const time_exitcode_t* te){
    if (te == NULL){
        dprintf(STDERR_FILENO, "ERROR time exitcode null\n");
        return;
    }

    // Conversion
    time_t t = (time_t)te->time;
    struct tm tm_info;
    localtime_r(&t, &tm_info);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);

    dprintf(STDOUT_FILENO, "%s : ", buffer);
    dprintf(STDOUT_FILENO, "%u\n", te->exitcode);
}

void all_time_show(time_array_t* te_arr){
    if(te_arr == NULL){
        dprintf(STDOUT_FILENO, "The array of times exitcodes is null\n");
    }
    for (uint32_t i = 0; i < te_arr -> nbruns; i++){
        time_exitcode_show(&(te_arr -> all_timecode)[i]);
    }
    
}