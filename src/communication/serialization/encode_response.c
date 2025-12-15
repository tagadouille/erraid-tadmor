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
    if (!ans)
        return -1;

    if (ans->anstype == (uint16_t)ERR) {
        if (encode_uint16(fd, (uint16_t)ERR) < 0)
            return -1;

        return encode_uint16(fd, ans->errcode);
    } else {
        if (encode_uint16(fd, (uint16_t)OK) < 0)
            return -1;

        if (encode_string(fd, &ans->output) < 0)
            return -1;

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
int encode_a_list(int fd, const a_list_t *ans)
{

    if (!ans) {
        dprintf(2, "[encode_a_list] ERROR: ans is NULL\n");
        return -1;
    }

    dprintf(2, "[encode_a_list] nbtask=%u\n", ans->all_task.nbtask);

    if (encode_uint16(fd, (uint16_t)OK) < 0) {
        dprintf(2, "[encode_a_list] ERROR: encode_uint16(OK) failed\n");
        return -1;
    }

    dprintf(2, "[encode_a_list] wrote anstype=OK\n");

    if (encode_uint32(fd, ans->all_task.nbtask) < 0) {
        dprintf(2, "[encode_a_list] ERROR: encode_uint32(nbtask) failed\n");
        return -1;
    }

    for (uint32_t i = 0; i < ans->all_task.nbtask; ++i) {
        const task_t *t = &ans->all_task.all_task[i];

        dprintf(2, "[encode_a_list] encoding task #%u\n", i);
        dprintf(2, "[encode_a_list] task[%u].id=%lu\n", i, t->id);

        if (encode_uint64(fd, t->id) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_uint64(id) failed (i=%u)\n", i);
            return -1;
        }

        if (!t->timing) {
            dprintf(2, "[encode_a_list] ERROR: timing is NULL (i=%u)\n", i);
            return -1;
        }

        if (encode_timing(fd, t->timing) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_timing failed (i=%u)\n", i);
            return -1;
        }

        dprintf(2, "[encode_a_list] timing encoded (i=%u)\n", i);

        string_t *cline = command_to_commandline(t->cmd);
        string_t empty = { .length = 0, .data = "" };

        if (!cline) {
            dprintf(2, "[encode_a_list] WARNING: empty commandline (i=%u)\n", i);
            if (encode_string(fd, &empty) < 0) {
                dprintf(2, "[encode_a_list] ERROR: encode_string(empty) failed\n");
                return -1;
            }
        } else {
            if (encode_string(fd, cline) < 0) {
                dprintf(2, "[encode_a_list] ERROR: encode_string failed (i=%u)\n", i);
                free(cline->data);
                free(cline);
                return -1;
            }
            free(cline->data);
            free(cline);
        }

        dprintf(2, "[encode_a_list] task #%u encoded\n", i);
    }

    dprintf(2, "[encode_a_list] SUCCESS\n");
    return 0;
}


int encode_answer(int fd, const answer_t *ans)
{
    if (!ans) return -1;

    // Écrire le type
    if (encode_uint16(fd, ans->anstype) < 0) return -1;

    if (ans->anstype == OK) {
        // OK = task_id
        return encode_uint64(fd, ans->task_id);
    }

    if (ans->anstype == ERR) {
        // ER = errcode
        return encode_uint16(fd, ans->errcode);
    }

    // Type inconnu
    return -1;
}


