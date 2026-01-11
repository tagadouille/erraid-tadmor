#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include "types/my_string.h"
#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

#include <stdio.h>
#include <stdlib.h>

/* STDOUT / STDERR responses:
   OK: ANSTYPE='OK' + OUTPUT<string>
   ERR: ANSTYPE='ER' + ERRCODE<uint16>
*/

int encode_a_output(int fd, const a_output_t *ans)
{
    if (!ans){
        dprintf(STDERR_FILENO, "Error : the answer in NULL at encode_a_output\n");
        return -1;
    }

    if (ans->anstype == (uint16_t)ERR) {
        if (encode_uint16(fd, (uint16_t)ERR) < 0){
            dprintf(STDERR_FILENO, "Error : an error occured while encoding ERR for encode_a_output\n");
            return -1;
        }

        return encode_uint16(fd, ans->errcode);
    } else {
        if (encode_uint16(fd, (uint16_t)OK) < 0){
            dprintf(STDERR_FILENO, "Error : an error occured while encoding OK for encode_a_output\n");
            return -1;
        }

        if (encode_string(fd, ans->output) < 0){
            dprintf(STDERR_FILENO, "Error : an error occured while encoding string for encode_a_output\n");
            return -1;
        }
        return 0;
    }
}

/* TIMES_EXITCODES:
   OK: ANSTYPE='OK' + NBRUNS(uint32) + N * (TIME int64 + EXITCODE uint16)
   ERR: ANSTYPE='ER' + ERRCODE(uint16)
*/

int encode_a_timecode(int fd, const a_timecode_t *a)
{
    if (!a)
        return -1;

    if (a->anstype == (uint16_t)ERR) {
        if (encode_uint16(fd, (uint16_t)ERR) < 0)
            return -1;

        return encode_uint16(fd, a->errcode);
    }

    if (encode_uint16(fd, (uint16_t)OK) < 0)
        return -1;

    if (encode_uint32(fd, a->time_arr.nbruns) < 0)
        return -1;

    for (uint32_t i = 0; i < a->time_arr.nbruns; ++i) {
        if (encode_int64(fd, a->time_arr.all_timecode[i].time) < 0)
            return -1;

        if (encode_uint16(fd, (uint16_t)a->time_arr.all_timecode[i].exitcode) < 0)
            return -1;
    }

    return 0;
}

/* LIST OK: ANSTYPE='OK' + NBTASKS(uint32) +
   for each task: TASKID(uint64), TIMING, COMMANDLINE(string)
*/
int encode_a_list(int fd, const a_list_t *ans) {
    
    if (!ans || !ans->all_task) {
        dprintf(2, "[encode_a_list] ERROR: ans or ans->all_task is NULL\n");
        return -1;
    }

    const uint32_t nbtask = ans->all_task->nbtask;
    const task_t *tasks = ans->all_task->all_task;

    dprintf(2, "[encode_a_list] nbtask=%u\n", nbtask);

    if (nbtask > 0 && !tasks) {
        dprintf(2, "[encode_a_list] ERROR: nbtask > 0 but all_task pointer is NULL\n");
        return -1;
    }

    /* ---------- answer type ---------- */
    if (encode_uint16(fd, (uint16_t)OK) < 0) {
        dprintf(2, "[encode_a_list] ERROR: encode_uint16(OK) failed\n");
        return -1;
    }
    dprintf(2, "[encode_a_list] wrote anstype=OK\n");

    /* ---------- number of tasks ---------- */
    if (encode_uint32(fd, nbtask) < 0) {
        dprintf(2, "[encode_a_list] ERROR: encode_uint32(nbtask) failed\n");
        return -1;
    }

    /* ---------- encode each task ---------- */
    for (uint32_t i = 0; i < nbtask; ++i) {

        const task_t *t = &tasks[i];

        dprintf(2, "[encode_a_list] encoding task #%u\n", i);

        /* --- id --- */
        dprintf(2, "[encode_a_list] task[%u].id=%lu\n", i, t->id);

        if (encode_uint64(fd, t->id) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_uint64(id) failed (i=%u)\n", i);
            return -1;
        }

        /* --- timing --- */
        if (!t->timing) {
            dprintf(2, "[encode_a_list] ERROR: timing is NULL (i=%u)\n", i);
            return -1;
        }

        if (encode_timing(fd, t->timing) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_timing failed (i=%u)\n", i);
            return -1;
        }

        dprintf(2, "[encode_a_list] timing encoded (i=%u)\n", i);

        /* --- command --- */
        if (!t->cmd) {
            dprintf(2, "[encode_a_list] ERROR: cmd is NULL (i=%u)\n", i);
            return -1;
        }

        if (encode_command(fd, t->cmd) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_command failed (i=%u)\n", i);
            return -1;
        }

        dprintf(2,"[encode_a_list] command encoded (i=%u)\n", i);

        dprintf(2, "[encode_a_list] task #%u encoded\n", i);
    }

    dprintf(2, "[encode_a_list] SUCCESS\n");
    return 0;
}

int encode_answer(int fd, const answer_t *ans)
{
    if (!ans){
        dprintf(2, "Error : The answer can't be null\n");
        return -1;
    }

    if (encode_uint16(fd, (uint16_t) ans->anstype) < 0){
        dprintf(2, "Error : an error occured while encoding anstype\n");
        return -1;
    }

    int ret = -1;

    if (ans->anstype == (uint16_t) OK) {
        ret = encode_uint64(fd, ans->task_id);

        if(ret < 0){
            dprintf(2, "Error : an error occured while encoding uint64\n");
        }
    }

    if (ans->anstype == (uint16_t) ERR) {
        ret = encode_uint16(fd, (uint16_t) ans->errcode);

        if(ret < 0){
            dprintf(2, "Error : an error occured while encoding uint16\n");
        }
    }

    return ret;
}


