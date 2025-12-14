#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

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
    if (!ans)
        return -1;

    uint16_t anstype;

    if (decode_uint16(fd, &anstype) < 0)
        return -1;

    if (anstype != (uint16_t)OK)
        return -1;

    if (decode_uint32(fd, &ans->all_task.nbtask) < 0)
        return -1;

    if (ans->all_task.nbtask > 0) {
        ans->all_task.all_task = calloc(ans->all_task.nbtask, sizeof(task_t));

        if (!ans->all_task.all_task)
            return -1;

        for (uint32_t i = 0; i < ans->all_task.nbtask; ++i) {

            if (decode_uint64(fd, &ans->all_task.all_task[i].id) < 0) {
                free(ans->all_task.all_task);
                return -1;
            }

            ans->all_task.all_task[i].timing = malloc(sizeof(timing_t));

            if (!ans->all_task.all_task[i].timing) {
                free(ans->all_task.all_task);
                return -1;
            }

            if (decode_timing(fd, ans->all_task.all_task[i].timing) < 0) {
                free(ans->all_task.all_task[i].timing);
                free(ans->all_task.all_task);
                return -1;
            }

            /* Commandline is a string -> decode into temporary string_t */
            string_t tmp;

            if (decode_string(fd, &tmp) < 0) {
                free(ans->all_task.all_task[i].timing);
                free(ans->all_task.all_task);
                return -1;
            }

            /* Here we don't reconstruct command_t; store commandline into task structure if you have a field for it.
               For now we can keep it in a temporary buffer (user code should free tmp.data). */
            /* Example: if task_t has field 'char *commandline', set it: */
            ans->all_task.all_task[i].commandline = tmp.data; /* NOTE: requires task_t has commandline field (char*) */
            ans->all_task.all_task[i].commandline_len = tmp.length; /* optional */
        }
    } else {
        ans->all_task.all_task = NULL;
    }

    return 0;
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
