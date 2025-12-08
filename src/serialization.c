#include "serialization.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h> // pour htonl, htons, htonll, ntohl, ntohs, ntohll
#include <errno.h>

/* ============================================================
 * LOW-LEVEL INTEGER ENCODING (BIG-ENDIAN)
 * ============================================================ */

int encode_uint8(int fd, uint8_t v) {
    return write(fd, &v, sizeof(uint8_t)) == sizeof(uint8_t) ? 0 : -1;
}

int decode_uint8(int fd, uint8_t *v) {
    return read(fd, v, sizeof(uint8_t)) == sizeof(uint8_t) ? 0 : -1;
}

int encode_uint16(int fd, uint16_t v) {
    uint16_t be = htons(v);
    return write(fd, &be, sizeof(be)) == sizeof(be) ? 0 : -1;
}

int decode_uint16(int fd, uint16_t *v) {
    uint16_t be;
    if(read(fd, &be, sizeof(be)) != sizeof(be)) return -1;
    *v = ntohs(be);
    return 0;
}

int encode_uint32(int fd, uint32_t v) {
    uint32_t be = htonl(v);
    return write(fd, &be, sizeof(be)) == sizeof(be) ? 0 : -1;
}

int decode_uint32(int fd, uint32_t *v) {
    uint32_t be;
    if(read(fd, &be, sizeof(be)) != sizeof(be)) return -1;
    *v = ntohl(be);
    return 0;
}

int encode_uint64(int fd, uint64_t v) {
    uint64_t be = htobe64(v);
    return write(fd, &be, sizeof(be)) == sizeof(be) ? 0 : -1;
}

int decode_uint64(int fd, uint64_t *v) {
    uint64_t be;
    if(read(fd, &be, sizeof(be)) != sizeof(be)) return -1;
    *v = be64toh(be);
    return 0;
}

int encode_int32(int fd, int32_t v) {
    int32_t be = htonl((uint32_t)v);
    return write(fd, &be, sizeof(be)) == sizeof(be) ? 0 : -1;
}

int decode_int32(int fd, int32_t *v) {
    int32_t be;
    if(read(fd, &be, sizeof(be)) != sizeof(be)) return -1;
    *v = (int32_t)ntohl((uint32_t)be);
    return 0;
}

/* ============================================================
 * STRING ENCODING (string_t)
 * format: LENGTH(uint32) + DATA[LENGTH]
 * ============================================================ */
int encode_string(int fd, const string_t *s) {
    if(!s) return -1;
    if(encode_uint32(fd, s->length) < 0) return -1;
    if(s->length > 0) {
        if(write(fd, s->data, s->length) != s->length) return -1;
    }
    return 0;
}

int decode_string(int fd, string_t *s) {
    if(!s) return -1;
    if(decode_uint32(fd, &s->length) < 0) return -1;
    if(s->length > 0) {
        s->data = malloc(s->length);
        if(!s->data) return -1;
        if(read(fd, s->data, s->length) != s->length) {
            free(s->data);
            return -1;
        }
    } else {
        s->data = NULL;
    }
    return 0;
}

/* ============================================================
 * TIMING ENCODING
 * MINUTES(uint64), HOURS(uint32), DAYS(uint8)
 * ============================================================ */
int encode_timing(int fd, const timing_t *t) {
    if(!t) return -1;
    if(encode_uint64(fd, t->minutes) < 0) return -1;
    if(encode_uint32(fd, t->hours) < 0) return -1;
    if(encode_uint8(fd, t->days_of_week) < 0) return -1;
    return 0;
}

int decode_timing(int fd, timing_t *t) {
    if(!t) return -1;
    if(decode_uint64(fd, &t->minutes) < 0) return -1;
    if(decode_uint32(fd, &t->hours) < 0) return -1;
    if(decode_uint8(fd, &t->days_of_week) < 0) return -1;
    return 0;
}

/* ============================================================
 * ARGUMENTS ENCODING
 * ARGC(uint32) + ARGV[i]=string
 * ============================================================ */
int encode_arguments(int fd, const arguments_t *args) {
    if(!args) return -1;
    if(encode_uint32(fd, args->argc) < 0) return -1;
    for(uint32_t i = 0; i < args->argc; i++) {
        if(encode_string(fd, &args->argv[i]) < 0) return -1;
    }
    return 0;
}

int decode_arguments(int fd, arguments_t *args) {
    if(!args) return -1;
    if(decode_uint32(fd, &args->argc) < 0) return -1;
    if(args->argc > 0) {
        args->argv = malloc(sizeof(string_t) * args->argc);
        if(!args->argv) return -1;
        for(uint32_t i = 0; i < args->argc; i++) {
            if(decode_string(fd, &args->argv[i]) < 0) return -1;
        }
    } else {
        args->argv = NULL;
    }
    return 0;
}

/* ============================================================
 * COMMAND ENCODING
 * Simple: TYPE='SI' + ARGS
 * Complex: TYPE + NBCMDS(uint32) + CMD[i]
 * ============================================================ */
int encode_command(int fd, const command_t *cmd) {
    if(!cmd) return -1;
    if(encode_uint16(fd, (uint16_t)cmd->type) < 0) return -1;
    if(cmd->type == SI) {
        return encode_arguments(fd, &cmd->args.simple);
    } else if(cmd->type == SQ) {
        if(encode_uint32(fd, cmd->args.composed.count) < 0) return -1;
        for(uint32_t i = 0; i < cmd->args.composed.count; i++) {
            if(encode_command(fd, cmd->args.composed.cmds[i]) < 0) return -1;
        }
    }
    return 0;
}

int decode_command(int fd, command_t *cmd) {
    uint16_t type;
    if(decode_uint16(fd, &type) < 0) return -1;
    cmd->type = (command_type_t)type;
    if(cmd->type == SI) {
        return decode_arguments(fd, &cmd->args.simple);
    } else if(cmd->type == SQ) {
        if(decode_uint32(fd, &cmd->args.composed.count) < 0) return -1;
        if(cmd->args.composed.count > 0) {
            cmd->args.composed.cmds = malloc(sizeof(command_t*) * cmd->args.composed.count);
            if(!cmd->args.composed.cmds) return -1;
            for(uint32_t i = 0; i < cmd->args.composed.count; i++) {
                cmd->args.composed.cmds[i] = malloc(sizeof(command_t));
                if(!cmd->args.composed.cmds[i]) return -1;
                if(decode_command(fd, cmd->args.composed.cmds[i]) < 0) return -1;
            }
        }
    }
    return 0;
}

/* ============================================================
 * SIMPLE REQUEST (LS, RM, SO, SE, TX, TM)
 * ============================================================ */
int encode_simple_request(int fd, const simple_request_t *req) {
    if(!req) return -1;
    if(encode_uint16(fd, req->opcode) < 0) return -1;
    if(req->opcode != LS && req->opcode != TM) {
        return encode_uint64(fd, req->task_id);
    }
    return 0;
}

int decode_simple_request(int fd, simple_request_t *req) {
    if(!req) return -1;
    if(decode_uint16(fd, &req->opcode) < 0) return -1;
    if(req->opcode != LS && req->opcode != TM) {
        if(decode_uint64(fd, &req->task_id) < 0) return -1;
    }
    return 0;
}

/* ============================================================
 * COMPLEX REQUEST (CR, CB)
 * ============================================================ */
int encode_complex_request(int fd, const complex_request_t *req) {
    if(!req) return -1;
    if(encode_uint16(fd, req->opcode) < 0) return -1;
    if(encode_timing(fd, &req->timing) < 0) return -1;
    if(req->opcode == CR) {
        return encode_command(fd, &req->u.command);
    } else if(req->opcode == CB) {
        if(encode_uint32(fd, req->u.composed.nb_task) < 0) return -1;
        for(uint32_t i = 0; i < req->u.composed.nb_task; i++) {
            if(encode_uint64(fd, req->u.composed.task_ids[i]) < 0) return -1;
        }
    }
    return 0;
}

int decode_complex_request(int fd, complex_request_t *req) {
    if(!req) return -1;
    if(decode_uint16(fd, &req->opcode) < 0) return -1;
    if(decode_timing(fd, &req->timing) < 0) return -1;
    if(req->opcode == CR) {
        return decode_command(fd, &req->u.command);
    } else if(req->opcode == CB) {
        if(decode_uint32(fd, &req->u.composed.nb_task) < 0) return -1;
        if(req->u.composed.nb_task > 0) {
            req->u.composed.task_ids = malloc(sizeof(uint64_t) * req->u.composed.nb_task);
            if(!req->u.composed.task_ids) return -1;
            for(uint32_t i = 0; i < req->u.composed.nb_task; i++) {
                if(decode_uint64(fd, &req->u.composed.task_ids[i]) < 0) return -1;
            }
        }
    }
    return 0;
}

/* ============================================================
 * ANSWERS (OK / ERR)
 * ============================================================ */
int encode_answer(int fd, const answer_t *ans) {
    if(!ans) return -1;
    if(encode_uint16(fd, ans->anstype) < 0) return -1;
    if(ans->anstype == ERR) {
        return encode_uint16(fd, ans->errcode);
    } else {
        return encode_uint64(fd, ans->task_id);
    }
}

int decode_answer(int fd, answer_t *ans) {
    if(!ans) return -1;
    if(decode_uint16(fd, &ans->anstype) < 0) return -1;
    if(ans->anstype == ERR) {
        return decode_uint16(fd, &ans->errcode);
    } else {
        return decode_uint64(fd, &ans->task_id);
    }
}

/* ============================================================
 * STDOUT / STDERR
 * ============================================================ */
int encode_a_output(int fd, const a_output_t *ans) {
    if(!ans) return -1;
    if(encode_uint16(fd, ans->anstype) < 0) return -1;
    if(ans->anstype == OK) {
        if(encode_string(fd, &ans->output) < 0) return -1;
    }
    return encode_uint16(fd, ans->errcode);
}

int decode_a_output(int fd, a_output_t *ans) {
    if(!ans) return -1;
    if(decode_uint16(fd, &ans->anstype) < 0) return -1;
    if(ans->anstype == OK) {
        if(decode_string(fd, &ans->output) < 0) return -1;
    }
    if(decode_uint16(fd, &ans->errcode) < 0) return -1;
    return 0;
}

/* ============================================================
 * TIME_EXITCODE
 * ============================================================ */
int encode_a_timecode(int fd, const a_timecode_t *ans) {
    if(!ans) return -1;
    if(encode_uint16(fd, ans->anstype) < 0) return -1;
    if(encode_uint32(fd, ans->nbrun) < 0) return -1;
    for(uint32_t i = 0; i < ans->nbrun; i++) {
        if(encode_uint64(fd, ans->all_timecode[i].time) < 0) return -1;
        if(encode_int32(fd, ans->all_timecode[i].exitcode) < 0) return -1;
    }
    return 0;
}

int decode_a_timecode(int fd, a_timecode_t *ans) {
    if(!ans) return -1;
    if(decode_uint16(fd, &ans->anstype) < 0) return -1;
    if(decode_uint32(fd, &ans->nbrun) < 0) return -1;
    if(ans->nbrun > 0) {
        ans->all_timecode = malloc(sizeof(time_exitcode_t) * ans->nbrun);
        if(!ans->all_timecode) return -1;
        for(uint32_t i = 0; i < ans->nbrun; i++) {
            if(decode_uint64(fd, &ans->all_timecode[i].time) < 0) return -1;
            if(decode_int32(fd, &ans->all_timecode[i].exitcode) < 0) return -1;
        }
    } else {
        ans->all_timecode = NULL;
    }
    return 0;
}

/* ============================================================
 * LIST
 * ============================================================ */
int encode_a_list(int fd, const a_list_t *ans) {
    if(!ans) return -1;
    if(encode_uint16(fd, ans->anstype) < 0) return -1;
    if(encode_uint32(fd, ans->nbtask) < 0) return -1;
    for(uint32_t i = 0; i < ans->nbtask; i++) {
        if(encode_uint64(fd, ans->all_task[i].id) < 0) return -1;
        if(encode_timing(fd, ans->all_task[i].timing) < 0) return -1;
        if(encode_command(fd, ans->all_task[i].cmd) < 0) return -1;
    }
    return 0;
}

int decode_a_list(int fd, a_list_t *ans) {
    if(!ans) return -1;
    if(decode_uint16(fd, &ans->anstype) < 0) return -1;
    if(decode_uint32(fd, &ans->nbtask) < 0) return -1;
    if(ans->nbtask > 0) {
        ans->all_task = malloc(sizeof(task_t) * ans->nbtask);
        if(!ans->all_task) return -1;
        for(uint32_t i = 0; i < ans->nbtask; i++) {
            if(decode_uint64(fd, &ans->all_task[i].id) < 0) return -1;
            ans->all_task[i].timing = malloc(sizeof(timing_t));
            if(!ans->all_task[i].timing) return -1;
            if(decode_timing(fd, ans->all_task[i].timing) < 0) return -1;
            ans->all_task[i].cmd = malloc(sizeof(command_t));
            if(!ans->all_task[i].cmd) return -1;
            if(decode_command(fd, ans->all_task[i].cmd) < 0) return -1;
        }
    } else {
        ans->all_task = NULL;
    }
    return 0;
}
