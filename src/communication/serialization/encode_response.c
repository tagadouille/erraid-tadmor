/* STDOUT / STDERR responses:
   OK: ANSTYPE='OK' + OUTPUT<string>
   ERR: ANSTYPE='ER' + ERRCODE<uint16>
*/
int encode_a_output(int fd, const a_output_t *ans)
{
    if (!ans) return -1;
    if (ans->anstype == (uint16_t)ERR) {
        if (encode_uint16(fd, (uint16_t)ERR) < 0) return -1;
        return encode_uint16(fd, ans->errcode);
    } else {
        if (encode_uint16(fd, (uint16_t)OK) < 0) return -1;
        if (encode_string(fd, &ans->output) < 0) return -1;
        return 0;
    }
}

/* TIMES_EXITCODES:
   OK: ANSTYPE='OK' + NBRUNS(uint32) + N * (TIME int64 + EXITCODE uint16)
   ERR: ANSTYPE='ER' + ERRCODE(uint16)
*/
int encode_a_timecode(int fd, const a_timecode_t *a)
{
    if (!a) return -1;
    if (a->anstype == (uint16_t)ERR) {
        if (encode_uint16(fd, (uint16_t)ERR) < 0) return -1;
        return encode_uint16(fd, a->errcode);
    }
    /* OK */
    if (encode_uint16(fd, (uint16_t)OK) < 0) return -1;
    if (encode_uint32(fd, a->nbrun) < 0) return -1;
    for (uint32_t i = 0; i < a->nbrun; ++i) {
        if (encode_int64(fd, a->all_timecode[i].time) < 0) return -1;
        if (encode_uint16(fd, (uint16_t)a->all_timecode[i].exitcode) < 0) return -1;
    }
    return 0;
}

/* LIST OK: ANSTYPE='OK' + NBTASKS(uint32) + for each task: TASKID(uint64), TIMING, COMMANDLINE(string) */
int encode_a_list(int fd, const a_list_t *ans)
{
    if (!ans) return -1;
    if (encode_uint16(fd, (uint16_t)OK) < 0) return -1;
    if (encode_uint32(fd, ans->nbtask) < 0) return -1;
    for (uint32_t i = 0; i < ans->nbtask; ++i) {
        /* task id */
        if (encode_uint64(fd, ans->all_task[i].id) < 0) return -1;
        /* timing */
        if (encode_timing(fd, ans->all_task[i].timing) < 0) return -1;
        /* commandline: build a single string from the command */
        string_t *cline = command_to_commandline(ans->all_task[i].cmd);
        if (!cline) return -1;
        if (encode_string(fd, cline) < 0) { free(cline->data); free(cline); return -1; }
        free(cline->data);
        free(cline);
    }
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
    if (encode_uint16(fd, (uint16_t)OK) < 0) return -1;
    return encode_uint64(fd, taskid);
}

/* Helper: encode an ERROR response: ANSTYPE='ER' (uint16) + ERRCODE (uint16) */
int encode_answer_err(int fd, uint16_t errcode)
{
    if (encode_uint16(fd, (uint16_t)ERR) < 0) return -1;
    return encode_uint16(fd, errcode);
}