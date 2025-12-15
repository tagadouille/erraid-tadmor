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

        output = string_create(data, length);

        return create_a_output_t(anstype, output, errcode);

    } else if (anstype == (uint16_t)OK) {
        anstype = (uint16_t)OK;

        if (decode_string(fd, &output) < 0)
            return NULL;

        errcode = 0;
        output = string_create(data, length);

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
    dprintf(2, "[decode_a_list] enter fd=%d\n", fd);

    uint16_t anstype = 0;
    uint32_t nbtask = 0;
    task_t* all_task = NULL;

    if (decode_uint16(fd, &anstype) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint16 failed\n");
        return NULL;
    }
    dprintf(2, "[decode_a_list] anstype=%u\n", anstype);

    /* Unknow case */
    if (anstype != (uint16_t)OK) {
        dprintf(2, "[decode_a_list] ERROR: anstype != OK (%u)\n", anstype);
        return NULL;
    }

    /* Ok case */
    if (decode_uint32(fd, &nbtask) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint32(nbtask) failed\n");
        return NULL;
    }

    dprintf(2, "[decode_a_list] nbtask=%u\n", nbtask);

    if (nbtask == 0) {
        all_task = NULL;
        dprintf(2, "[decode_a_list] nbtask == 0, nothing to decode\n");
        return create_a_list(anstype, nbtask, all_task);
    }

    all_task = calloc(nbtask, sizeof(task_t));
    if (!all_task) {
        dprintf(2, "[decode_a_list] ERROR: calloc(all_task) failed\n");
        return NULL;
    }

    for (uint32_t i = 0; i < nbtask; ++i) {
        task_t *t = &all_task[i];

        dprintf(2, "[decode_a_list] decoding task #%u\n", i);

        if (decode_uint64(fd, &t->id) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_uint64(id) failed (i=%u)\n", i);
            goto error;
        }
        dprintf(2, "[decode_a_list] task[%u].id=%lu\n", i, t->id);

        t->timing = malloc(sizeof(timing_t));
        if (!t->timing) {
            dprintf(2, "[decode_a_list] ERROR: malloc(timing) failed (i=%u)\n", i);
            goto error;
        }

        if (decode_timing(fd, t->timing) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_timing failed (i=%u)\n", i);
            goto error;
        }
        dprintf(2, "[decode_a_list] timing decoded (i=%u)\n", i);

        string_t tmp = {0};
        if (decode_string(fd, &tmp) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_string failed (i=%u)\n", i);
            goto error;
        }

        dprintf(2, "[decode_a_list] commandline len=%u data='%.*s'\n",
                tmp.length,
                tmp.length,
                tmp.data ? tmp.data : "(null)");

        /* ⚠️ ownership transféré ici */
        t->commandline = tmp.data;
        t->commandline_len = tmp.length;
    }

    dprintf(2, "[decode_a_list] SUCCESS\n");
    return create_a_list(anstype, nbtask, all_task);

    error:
    return NULL;
}

answer_t* decode_answer(int fd)
{

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
