#include <stdint.h>
#include "types/my_string.h"
#include "communication/answer.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

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

#include <stdio.h>
#include <unistd.h>

int encode_a_list(int fd, const a_list_t *ans)
{
    dprintf(2, "[encode_a_list] enter fd=%d ans=%p\n", fd, ans);

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

    dprintf(2, "[encode_a_list] wrote nbtask=%u\n", ans->all_task.nbtask);

    for (uint32_t i = 0; i < ans->all_task.nbtask; ++i) {

        dprintf(2, "[encode_a_list] encoding task #%u\n", i);

        dprintf(2, "[encode_a_list] task[%u].id=%lu\n",
                i, ans->all_task.all_task[i].id);

        if (encode_uint64(fd, ans->all_task.all_task[i].id) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_uint64(id) failed (i=%u)\n", i);
            return -1;
        }

        if (!ans->all_task.all_task[i].timing) {
            dprintf(2, "[encode_a_list] ERROR: timing is NULL (i=%u)\n", i);
            return -1;
        }

        if (encode_timing(fd, ans->all_task.all_task[i].timing) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_timing failed (i=%u)\n", i);
            return -1;
        }

        dprintf(2, "[encode_a_list] timing encoded (i=%u)\n", i);

        if (!ans->all_task.all_task[i].cmd) {
            dprintf(2, "[encode_a_list] ERROR: cmd is NULL (i=%u)\n", i);
            return -1;
        }

        string_t *cline =
            command_to_commandline(ans->all_task.all_task[i].cmd);

        if (!cline) {
            dprintf(2, "[encode_a_list] ERROR: command_to_commandline returned NULL (i=%u)\n", i);
            return -1;
        }

        dprintf(2,
            "[encode_a_list] commandline len=%u data='%.*s'\n",
            cline->length,
            cline->length,
            cline->data ? cline->data : "(null)"
        );

        if (encode_string(fd, cline) < 0) {
            dprintf(2, "[encode_a_list] ERROR: encode_string failed (i=%u)\n", i);
            free(cline->data);
            free(cline);
            return -1;
        }

        free(cline->data);
        free(cline);

        dprintf(2, "[encode_a_list] task #%u encoded\n", i);
    }

    dprintf(2, "[encode_a_list] SUCCESS\n");
    return 0;
}

/* REMOVE OK and TERMINATE OK are just ANSTYPE='OK' (no payload) */
int encode_answer_ok_nopayload(int fd)
{
    return encode_uint16(fd, (uint16_t)OK);
}

/* CREATE / COMBINE OK: ANSTYPE='OK' + TASKID(uint64) */
int encode_answer_ok_taskid(int fd, uint64_t taskid)
{
    if (encode_uint16(fd, (uint16_t)OK) < 0)
        return -1;

    return encode_uint64(fd, taskid);
}

/* Helper: encode an ERROR response:
   ANSTYPE='ER' (uint16) + ERRCODE (uint16)
*/
int encode_answer_err(int fd, uint16_t errcode)
{
    if (encode_uint16(fd, (uint16_t)ERR) < 0)
        return -1;

    return encode_uint16(fd, errcode);
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


