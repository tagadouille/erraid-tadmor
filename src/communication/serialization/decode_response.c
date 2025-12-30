#define _POSIX_C_SOURCE 200809L

#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdlib.h>
#include <stdio.h>

a_output_t* decode_a_output(int fd)
{

    uint16_t anstype = 0;
    uint16_t errcode = 0;

    uint32_t length = 0;
    char* data = NULL;

    string_t output = {0};

    if (decode_uint16(fd, &anstype) < 0)
        return NULL;

    if (anstype == (uint16_t)ERR) {
        uint16_t ec;

        if (decode_uint16(fd, &ec) < 0)
            return NULL;

        anstype = (uint16_t)ERR;
        errcode = ec;
        length = 0;
        data = NULL;

        output = string_create_from_cstr(data, length);

        return create_a_output_t(anstype, output, errcode);

    } else if (anstype == (uint16_t)OK) {
        anstype = (uint16_t)OK;

        if (decode_string(fd, &output) < 0)
            return NULL;

        errcode = 0;

        return create_a_output_t(anstype, output, errcode);
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
    dprintf(2, "[decode_a_list] enter fd=%d", fd);

    uint16_t anstype = 0;
    uint32_t nbtask = 0;
    task_t *all_task = NULL;

    /* ---------- decode answer type ---------- */
    if (decode_uint16(fd, &anstype) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint16 failed");
        return NULL;
    }
    dprintf(2, "[decode_a_list] anstype=%u", anstype);

    if (anstype != (uint16_t)OK) {
        dprintf(2, "[decode_a_list] ERROR: anstype != OK (%u)", anstype);
        return NULL;
    }

    /* ---------- decode number of tasks ---------- */
    if (decode_uint32(fd, &nbtask) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint32(nbtask) failed");
        return NULL;
    }
    dprintf(2, "[decode_a_list] nbtask=%u", nbtask);

    if (nbtask == 0) {
        dprintf(2, "[decode_a_list] nbtask == 0, nothing to decode");
        return create_a_list(anstype, nbtask, NULL);
    }

    /* ---------- allocate tasks ---------- */
    all_task = calloc(nbtask, sizeof(task_t));
    if (!all_task) {
        perror("[decode_a_list] calloc(all_task)");
        return NULL;
    }

    /* ---------- decode each task ---------- */
    for (uint32_t i = 0; i < nbtask; ++i) {
        task_t *t = &all_task[i];

        dprintf(2, "[decode_a_list] decoding task #%u", i);

        /* --- id --- */
        if (decode_uint64(fd, &t->id) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_uint64(id) failed (i=%u)", i);
            goto error;
        }
        dprintf(2, "[decode_a_list] task[%u].id=%lu", i, t->id);

        /* --- timing --- */
        t->timing = malloc(sizeof(timing_t));
        if (!t->timing) {
            perror("[decode_a_list] malloc(timing)");
            goto error;
        }
        if (decode_timing(fd, t->timing) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_timing failed (i=%u)", i);
            goto error;
        }
        dprintf(2, "[decode_a_list] timing decoded (i=%u)", i);

        /* --- command (all types) --- */
        if (decode_command(fd, &t->cmd) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_command failed (i=%u)", i);
            goto error;
        }
        dprintf(2, "[decode_a_list] command decoded (i=%u)", i);
    }

    dprintf(2, "[decode_a_list] SUCCESS\n");
    return create_a_list(anstype, nbtask, all_task);

    error:
    dprintf(2, "[decode_a_list] CLEANUP after error");
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
