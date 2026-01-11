#define _POSIX_C_SOURCE 200809L

#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdlib.h>
#include <stdio.h>

a_output_t* decode_a_output(int fd)
{
    uint16_t anstype = 0;
    if (decode_uint16(fd, &anstype) < 0) {
        dprintf(STDERR_FILENO, "[decode_a_output] Error: can't decode uint16 anstype\n");
        return NULL;
    }

    if (anstype == (uint16_t)ERR) {
        uint16_t errcode = 0;
        if (decode_uint16(fd, &errcode) < 0) {
            dprintf(STDERR_FILENO, "[decode_a_output] Error: can't decode uint16 errcode\n");
            return NULL;
        }

        // Create an empty string
        string_t *output_ptr = string_create(NULL, 0);
        if (!output_ptr) {
            return NULL;
        }

        a_output_t *a_output = create_a_output_t(anstype, output_ptr, errcode);
        string_free(output_ptr);
        return a_output;

    } else if (anstype == (uint16_t)OK) {

        string_t *output_ptr = decode_string(fd);
        if (!output_ptr) {
            dprintf(2, "[decode_a_output] Error: can't decode output string\n");
            return NULL;
        }

        a_output_t *a_output = create_a_output_t(anstype, output_ptr, 0);
        string_free(output_ptr);
        return a_output;
    }
    return NULL;
}

a_timecode_t* decode_a_timecode(int fd)
{

    uint16_t anstype = 0;
    uint32_t nbruns = 0;
    time_exitcode_t* all_timecode = NULL;

    if (decode_uint16(fd, &anstype) < 0)
        return NULL;

    if (anstype == (uint16_t)ERR) {
        uint16_t ec;

        if (decode_uint16(fd, &ec) < 0)
            return NULL;

        anstype = (uint16_t)ERR;
        nbruns = 0;

        return create_a_timecode_t(anstype, nbruns, all_timecode);
    }
    else if (anstype == (uint16_t)OK) {

        if (decode_uint32(fd, &nbruns) < 0)
            return NULL;

        if (nbruns > LIMIT_MAX_RUNS)
            return NULL;

        if (nbruns > 0) {
            all_timecode = calloc(nbruns, sizeof(time_exitcode_t));

            if (!all_timecode)
                return NULL;

            for (uint32_t i = 0; i < nbruns; ++i) {

                int64_t tmp;

                if (decode_int64(fd, &tmp) < 0) {
                    free(all_timecode);
                    return NULL;
                }

                all_timecode[i].time = tmp;

                uint16_t ec;

                if (decode_uint16(fd, &ec) < 0) {
                    free(all_timecode);
                    return NULL;
                }

                all_timecode[i].exitcode = (int32_t)ec;
            }
        } else {
            all_timecode = NULL;
        }

        return create_a_timecode_t(anstype, nbruns, all_timecode);
    }

    return NULL;
}

a_list_t* decode_a_list(int fd)
{

    uint16_t anstype = 0;
    uint32_t nbtask = 0;
    task_t *all_task = NULL;

    /* ---------- decode answer type ---------- */
    if (decode_uint16(fd, &anstype) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint16 failed\n");
        return NULL;
    }

    if (anstype != (uint16_t)OK) {
        dprintf(2, "[decode_a_list] ERROR: anstype != OK (%u)\n", anstype);
        return NULL;
    }

    /* ---------- decode number of tasks ---------- */
    if (decode_uint32(fd, &nbtask) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint32(nbtask) failed\n");
        return NULL;
    }

    if (nbtask == 0) {
        dprintf(2, "[decode_a_list] nbtask == 0, nothing to decode\n");
        return create_a_list(anstype, nbtask, NULL);
    }

    /* ---------- allocate tasks ---------- */
    all_task = calloc(nbtask, sizeof(task_t));
    if (!all_task) {
        perror("[decode_a_list] calloc(all_task)\n");
        return NULL;
    }

    /* ---------- decode each task ---------- */
    for (uint32_t i = 0; i < nbtask; ++i) {
        task_t *t = &all_task[i];

        /* --- id --- */
        if (decode_uint64(fd, &t->id) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_uint64(id) failed (i=%u)\n", i);
            goto error;
        }

        /* --- timing --- */
        t->timing = malloc(sizeof(timing_t));
        if (!t->timing) {
            perror("[decode_a_list] malloc(timing)");
            goto error;
        }
        if (decode_timing(fd, t->timing) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_timing failed (i=%u)\n", i);
            goto error;
        }

        /* --- command (all types) --- */
        if (decode_command(fd, &t->cmd) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_command failed (i=%u)\n", i);
            goto error;
        }
    }
    return create_a_list(anstype, nbtask, all_task);

    error:
    dprintf(2, "[decode_a_list] CLEANUP after error\n");
    if (all_task) {
        for (uint32_t j = 0; j < nbtask; ++j) {
            free(all_task[j].timing);
            command_free(all_task[j].cmd);
        }
        free(all_task);
    }
    return NULL;
}

answer_t* decode_answer(int fd){

    uint16_t anstype;
    uint16_t errcode;
    uint64_t task_id;

    if (decode_uint16(fd, &anstype) < 0) return NULL;

    if (anstype == OK) {
        if (decode_uint64(fd, &task_id) < 0) return NULL;
        errcode = 0;
    }

    if (anstype == ERR) {
        task_id = 0;
        if (decode_uint16(fd, &errcode) < 0) return NULL;
    }

    answer_t* ans = create_answer(anstype, task_id, errcode);

    return ans;
}
