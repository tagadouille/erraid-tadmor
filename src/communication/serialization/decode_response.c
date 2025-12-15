#define _POSIX_C_SOURCE 200809L

#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdlib.h>
#include <stdio.h>

int decode_a_output(int fd, a_output_t *ans)
{
    if (!ans)
        return -1;

    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    if (anstype == (uint16_t)ERR) {
        uint16_t ec;

        if (decode_uint16(fd, &ec) < 0)
            return -1;

        ans->anstype = (uint16_t)ERR;
        ans->errcode = ec;
        ans->output.length = 0;
        ans->output.data = NULL;

        return 0;
    } else if (anstype == (uint16_t)OK) {
        ans->anstype = (uint16_t)OK;

        if (decode_string(fd, &ans->output) < 0)
            return -1;

        ans->errcode = 0;

        return 0;
    }

    return -1;
}

int decode_a_timecode(int fd, a_timecode_t *a)
{
    if (!a)
        return -1;

    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    if (anstype == (uint16_t)ERR) {
        uint16_t ec;

        if (decode_uint16(fd, &ec) < 0)
            return -1;

        a->anstype = (uint16_t)ERR;
        a->errcode = ec;
        a->time_arr.nbruns = 0;

        return 0;
    }
    else if (anstype == (uint16_t)OK) {
        a->anstype = (uint16_t)OK;

        if (decode_uint32(fd, &a->time_arr.nbruns) < 0)
            return -1;

        if (a->time_arr.nbruns > LIMIT_MAX_RUNS)
            return -1;

        if (a->time_arr.nbruns > 0) {
            a->time_arr.all_timecode = calloc(a->time_arr.nbruns, sizeof(time_exitcode_t));

            if (!a->time_arr.all_timecode)
                return -1;

            for (uint32_t i = 0; i < a->time_arr.nbruns; ++i) {

                int64_t tmp;

                if (decode_int64(fd, &tmp) < 0) {
                    free(a->time_arr.all_timecode);
                    return -1;
                }

                a->time_arr.all_timecode[i].time = tmp;

                uint16_t ec;

                if (decode_uint16(fd, &ec) < 0) {
                    free(a->time_arr.all_timecode);
                    return -1;
                }

                a->time_arr.all_timecode[i].exitcode =
                    (int32_t)ec;
            }
        } else {
            a->time_arr.all_timecode = NULL;
        }

        return 0;
    }

    return -1;
}

int decode_a_list(int fd, a_list_t *ans)
{
    dprintf(2, "[decode_a_list] enter fd=%d ans=%p\n", fd, ans);

    if (!ans) {
        dprintf(2, "[decode_a_list] ERROR: ans is NULL\n");
        return -1;
    }

    /* Toujours partir d’un état propre */
    memset(ans, 0, sizeof(*ans));

    uint16_t anstype = 0;

    if (decode_uint16(fd, &anstype) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint16 failed\n");
        return -1;
    }

    ans->anstype = anstype;
    dprintf(2, "[decode_a_list] anstype=%u\n", anstype);

    /* Error Case */
    if (anstype == (uint16_t)ERR) {
        if (decode_uint16(fd, &ans->errcode) < 0) {
            dprintf(2, "[decode_a_list] ERROR: decode_uint16(errcode) failed\n");
            return -1;
        }
        dprintf(2, "[decode_a_list] ERROR response, errcode=%u\n", ans->errcode);
        return 0;
    }

    /* Unknow case */
    if (anstype != (uint16_t)OK) {
        dprintf(2, "[decode_a_list] ERROR: anstype != OK (%u)\n", anstype);
        return -1;
    }

    /* Ok case */
    if (decode_uint32(fd, &ans->all_task.nbtask) < 0) {
        dprintf(2, "[decode_a_list] ERROR: decode_uint32(nbtask) failed\n");
        return -1;
    }

    dprintf(2, "[decode_a_list] nbtask=%u\n", ans->all_task.nbtask);

    if (ans->all_task.nbtask == 0) {
        ans->all_task.all_task = NULL;
        dprintf(2, "[decode_a_list] nbtask == 0, nothing to decode\n");
        return 0;
    }

    ans->all_task.all_task = calloc(ans->all_task.nbtask, sizeof(task_t));
    if (!ans->all_task.all_task) {
        dprintf(2, "[decode_a_list] ERROR: calloc(all_task) failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < ans->all_task.nbtask; ++i) {
        task_t *t = &ans->all_task.all_task[i];

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
    return 0;

error:
    free_a_list(ans);
    return -1;
}


int decode_answer_ok_nopayload(int fd)
{
    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    return (anstype == (uint16_t)OK) ? 0 : -1;
}

int decode_answer_ok_taskid(int fd, uint64_t *taskid_out)
{
    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    if (anstype != (uint16_t)OK)
        return -1;

    if (decode_uint64(fd, taskid_out) < 0)
        return -1;

    return 0;
}

/* Helper: decode ERROR response */
int decode_answer_err(int fd, uint16_t *errcode_out)
{
    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    if (anstype != (uint16_t)ERR)
        return -1;

    if (decode_uint16(fd, errcode_out) < 0)
        return -1;

    return 0;
}

int decode_answer(int fd, answer_t *ans)
{
    if (!ans) return -1;

    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;

    ans->anstype = anstype;

    if (anstype == OK) {
        if (decode_uint64(fd, &ans->task_id) < 0) return -1;
        ans->errcode = 0;
        return 0;
    }

    if (anstype == ERR) {
        ans->task_id = 0;
        if (decode_uint16(fd, &ans->errcode) < 0) return -1;
        return 0;
    }

    return -1;
}
