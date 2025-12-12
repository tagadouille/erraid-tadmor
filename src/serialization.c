/* serialization.c
 *
 * Serialization / deserialization
 *
 * - All numeric fields are big-endian on the wire.
 * - All I/O uses write_full / read_full to avoid partial reads/writes.
 * - Conforms to formats described in protocole.md:
 *     Requests: LS, CR, CB, RM, TX, SO, SE, TM
 *     Responses: OK / ER variants (LIST, CREATE, COMBINE, REMOVE, TIMES_EXITCODES, STDOUT/STDERR, TERMINATE)
 *
 * Note: this file assumes the following helper types exist (adjust if different):
 *   string_t { uint32_t length; char *data; }
 *   arguments_t { string_t *command; uint32_t argc; string_t **argv; }
 *   timing_t { uint64_t minutes; uint32_t hours; uint8_t daysofweek; }
 *   command_t { command_type_t type; union { arguments_t simple; struct { uint16_t count; command_t **cmds; } composed; } args; }
 *   a_list_t, a_timecode_t, a_output_t, answer_t, simple_request_t, complex_request_t exist as in your headers.
 *
 * If your real types differ, update small sections accordingly.
 */

#define _POSIX_C_SOURCE 200809L
#include "serialization.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> /* htons/htonl */
#include <stdint.h>
#include <limits.h>
#include <stdio.h>

/* Safety limits to avoid malicious / corrupted sizes */
#define LIMIT_MAX_STR_LEN   (1u << 20)   /* 1 MiB */
#define LIMIT_MAX_ARGC      10000u
#define LIMIT_MAX_CMDS      10000u
#define LIMIT_MAX_TASKS     100000u
#define LIMIT_MAX_RUNS      100000u

/* --------------------- low-level robust I/O --------------------- */
/* returns 0 on success, -1 on error */
static int write_full(int fd, const void *buf, size_t size)
{
    const unsigned char *p = buf;
    size_t off = 0;
    while (off < size) {
        ssize_t w = write(fd, p + off, size - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (w == 0) return -1; /* shouldn't happen for pipes when writing */
        off += (size_t)w;
    }
    return 0;
}

static int read_full(int fd, void *buf, size_t size)
{
    unsigned char *p = buf;
    size_t off = 0;
    while (off < size) {
        ssize_t r = read(fd, p + off, size - off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1; /* EOF => incomplete message */
        off += (size_t)r;
    }
    return 0;
}

/* --------------------- portable 64-bit BE helpers --------------------- */
static uint64_t hton64(uint64_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)(x & 0xFFFFFFFFu);
    hi = htonl(hi);
    lo = htonl(lo);
    return ((uint64_t)lo << 32) | hi;
#else
    return x;
#endif
}
static uint64_t ntoh64(uint64_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t lo = (uint32_t)(x & 0xFFFFFFFFu);
    hi = ntohl(hi);
    lo = ntohl(lo);
    return ((uint64_t)lo << 32) | hi;
#else
    return x;
#endif
}

/* --------------------- primitive encoders/decoders --------------------- */

int encode_uint8(int fd, uint8_t v) { return write_full(fd, &v, sizeof(v)); }
int decode_uint8(int fd, uint8_t *v) { return read_full(fd, v, sizeof(*v)); }

int encode_uint16(int fd, uint16_t v) {
    uint16_t be = htons(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint16(int fd, uint16_t *v) {
    uint16_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) return -1;
    *v = ntohs(be);
    return 0;
}

int encode_uint32(int fd, uint32_t v) {
    uint32_t be = htonl(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint32(int fd, uint32_t *v) {
    uint32_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) return -1;
    *v = ntohl(be);
    return 0;
}

int encode_uint64(int fd, uint64_t v) {
    uint64_t be = hton64(v);
    return write_full(fd, &be, sizeof(be));
}
int decode_uint64(int fd, uint64_t *v) {
    uint64_t be;
    if (read_full(fd, &be, sizeof(be)) < 0) return -1;
    *v = ntoh64(be);
    return 0;
}

int encode_int64(int fd, int64_t v) {
    return encode_uint64(fd, (uint64_t)v);
}
int decode_int64(int fd, int64_t *v) {
    uint64_t u;
    if (decode_uint64(fd, &u) < 0) return -1;
    *v = (int64_t)u;
    return 0;
}

/* signed 32 bits support (if needed) */
int encode_int32(int fd, int32_t v) {
    uint32_t u = (uint32_t)v;
    return encode_uint32(fd, u);
}
int decode_int32(int fd, int32_t *v) {
    uint32_t u;
    if (decode_uint32(fd, &u) < 0) return -1;
    *v = (int32_t)u;
    return 0;
}

/* --------------------- string encoding ---------------------
   Wire format: LENGTH (uint32) followed by LENGTH bytes (no terminal 0).
   Here we assume string_t { uint32_t length; char *data; }.
-----------------------------------------------------------------*/

int encode_string(int fd, const string_t *s) {
    if (!s) return -1;
    if (s->length > LIMIT_MAX_STR_LEN) return -1;
    if (encode_uint32(fd, s->length) < 0) return -1;
    if (s->length > 0) {
        if (write_full(fd, s->data, s->length) < 0) return -1;
    }
    return 0;
}

int decode_string(int fd, string_t *s) {
    if (!s) return -1;
    if (decode_uint32(fd, &s->length) < 0) return -1;
    if (s->length > LIMIT_MAX_STR_LEN) return -1;
    if (s->length > 0) {
        s->data = malloc((size_t)s->length + 1);
        if (!s->data) return -1;
        if (read_full(fd, s->data, s->length) < 0) {
            free(s->data);
            return -1;
        }
        s->data[s->length] = '\0';
    } else {
        s->data = NULL;
    }
    return 0;
}

/* --------------------- timing encoding --------------------- */
/* timing: MINUTES(uint64), HOURS(uint32), DAYSOFWEEK(uint8) */

int encode_timing(int fd, const timing_t *t) {
    if (!t) return -1;
    if (encode_uint64(fd, t->minutes) < 0) return -1;
    if (encode_uint32(fd, t->hours) < 0) return -1;
    if (encode_uint8(fd, t->daysofweek) < 0) return -1;
    return 0;
}
int decode_timing(int fd, timing_t *t) {
    if (!t) return -1;
    if (decode_uint64(fd, &t->minutes) < 0) return -1;
    if (decode_uint32(fd, &t->hours) < 0) return -1;
    if (decode_uint8(fd, &t->daysofweek) < 0) return -1;
    return 0;
}

/* --------------------- arguments encoding ---------------------
 * Protocol: ARGC (uint32), then ARGV[0..ARGC-1] as strings.
 * Note: for CREATE request the COMMAND is exactly this arguments_t per protocol.
 * In your in-memory arguments_t we assume fields: uint32_t argc; string_t **argv;
 * (no separate command field). If your structure differs, adapt accordingly.
 -----------------------------------------------------------------*/

int encode_arguments(int fd, const arguments_t *args) {
    if (!args) return -1;
    if (args->argc == 0 || args->argc > LIMIT_MAX_ARGC) return -1;
    if (encode_uint32(fd, args->argc) < 0) return -1;
    for (uint32_t i = 0; i < args->argc; ++i) {
        if (encode_string(fd, args->argv[i]) < 0) return -1;
    }
    return 0;
}

int decode_arguments(int fd, arguments_t *args) {
    if (!args) return -1;
    uint32_t argc;
    if (decode_uint32(fd, &argc) < 0) return -1;
    if (argc == 0 || argc > LIMIT_MAX_ARGC) return -1;
    args->argc = argc;
    args->argv = calloc((size_t)argc, sizeof(string_t*));
    if (!args->argv) return -1;
    for (uint32_t i = 0; i < argc; ++i) {
        args->argv[i] = malloc(sizeof(string_t));
        if (!args->argv[i]) {
            /* cleanup */
            for (uint32_t j = 0; j < i; ++j) {
                free(args->argv[j]->data);
                free(args->argv[j]);
            }
            free(args->argv);
            return -1;
        }
        if (decode_string(fd, args->argv[i]) < 0) {
            /* cleanup */
            for (uint32_t j = 0; j <= i; ++j) {
                if (args->argv[j]) { free(args->argv[j]->data); free(args->argv[j]); }
            }
            free(args->argv);
            return -1;
        }
    }
    return 0;
}

/* --------------------- helper: build commandline string ----------
 * Protocol LIST wants TASK.N.COMMANDLINE as a single string.
 * If command is simple (SI) we join argv with spaces.
 * If command is composed (SQ) we flatten subcommands separated by " | " (or " ; ").
 * We return a freshly malloced string_t (filled) or NULL on error.
 * Caller must free->data and struct.
 -----------------------------------------------------------------*/
static string_t *command_to_commandline(const command_t *cmd)
{
    if (!cmd) return NULL;
    /* If simple: join argv strings */
    if (cmd->type == SI) {
        /* compute length */
        size_t total = 0;
        uint32_t argc = cmd->args.simple.argc;
        if (argc == 0) return NULL;
        for (uint32_t i = 0; i < argc; ++i) {
            string_t *s = cmd->args.simple.argv[i];
            if (!s) return NULL;
            total += s->length;
            if (i + 1 < argc) total += 1; /* space */
        }
        if (total > LIMIT_MAX_STR_LEN) return NULL;
        string_t *out = malloc(sizeof(string_t));
        if (!out) return NULL;
        out->length = (uint32_t)total;
        out->data = malloc(total + 1);
        if (!out->data) { free(out); return NULL; }
        size_t off = 0;
        for (uint32_t i = 0; i < argc; ++i) {
            string_t *s = cmd->args.simple.argv[i];
            memcpy(out->data + off, s->data, s->length);
            off += s->length;
            if (i + 1 < argc) {
                out->data[off++] = ' ';
            }
        }
        out->data[off] = '\0';
        return out;
    } else {
        /* composed: build by concatenating subcommands with ' ; ' */
        size_t total = 0;
        uint16_t n = cmd->args.composed.count;
        if (n == 0) return NULL;
        string_t **subs = calloc(n, sizeof(string_t*));
        if (!subs) return NULL;
        for (uint16_t i = 0; i < n; ++i) {
            subs[i] = command_to_commandline(cmd->args.composed.cmds[i]);
            if (!subs[i]) {
                for (uint16_t j = 0; j < i; ++j) {
                    free(subs[j]->data);
                    free(subs[j]);
                }
                free(subs);
                return NULL;
            }
            total += subs[i]->length;
            if (i + 1 < n) total += 3; /* " ; " */
        }
        if (total > LIMIT_MAX_STR_LEN) {
            for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); }
            free(subs);
            return NULL;
        }
        string_t *out = malloc(sizeof(string_t));
        if (!out) { for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); } free(subs); return NULL; }
        out->length = (uint32_t)total;
        out->data = malloc(total + 1);
        if (!out->data) { free(out); for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); } free(subs); return NULL; }
        size_t off = 0;
        for (uint16_t i = 0; i < n; ++i) {
            memcpy(out->data + off, subs[i]->data, subs[i]->length);
            off += subs[i]->length;
            if (i + 1 < n) {
                out->data[off++] = ' ';
                out->data[off++] = ';';
                out->data[off++] = ' ';
            }
            free(subs[i]->data);
            free(subs[i]);
        }
        free(subs);
        out->data[off] = '\0';
        return out;
    }
}

/* --------------------- request encoding/decoding ---------------------
   Per protocole.md
---------------------------------------------------------------------*/

/* Simple request: OPCODE(uint16) + optional TASKID(uint64) */
int encode_simple_request(int fd, const simple_request_t *r)
{
    if (!r) return -1;
    if (encode_uint16(fd, r->opcode) < 0) return -1;
    /* LS and TM have no task id; other opcodes have task id */
    if (r->opcode == LS || r->opcode == TM) return 0;
    return encode_uint64(fd, r->task_id);
}

int decode_simple_request(int fd, simple_request_t *r)
{
    if (!r) return -1;
    if (decode_uint16(fd, &r->opcode) < 0) return -1;
    if (r->opcode == LS || r->opcode == TM) return 0;
    if (decode_uint64(fd, &r->task_id) < 0) return -1;
    return 0;
}

/* Complex request:
   - CR: OPCODE + TIMING + COMMAND (arguments)
   - CB: OPCODE + TIMING + TYPE(uint16) + NBTASKS(uint32) + TASKID[...]
*/
/*int encode_complex_request(int fd, const complex_request_t *r)
{
    if (!r) return -1;
    if (encode_uint16(fd, r->opcode) < 0) return -1;
    if (encode_timing(fd, &r->timing) < 0) return -1;

    if (r->opcode == CR) {
        //COMMAND = arguments
        if (encode_arguments(fd, &r->u.command_args) < 0) return -1;
        return 0;
    } else if (r->opcode == CB) {
        if (encode_uint16(fd, r->u.combine_type) < 0) return -1;
        if (r->u.nb_task > LIMIT_MAX_TASKS) return -1;
        if (encode_uint32(fd, r->u.nb_task) < 0) return -1;
        for (uint32_t i = 0; i < r->u.nb_task; ++i) {
            if (encode_uint64(fd, r->u.task_ids[i]) < 0) return -1;
        }
        return 0;
    }
    return -1;
}

int decode_complex_request(int fd, complex_request_t *r)
{
    if (!r) return -1;
    if (decode_uint16(fd, &r->opcode) < 0) return -1;
    if (decode_timing(fd, &r->timing) < 0) return -1;

    if (r->opcode == CR) {
        // decode arguments into u.command_args
        if (decode_arguments(fd, &r->u.command_args) < 0) return -1;
        return 0;
    } else if (r->opcode == CB) {
        if (decode_uint16(fd, &r->u.combine_type) < 0) return -1;
        uint32_t n;
        if (decode_uint32(fd, &n) < 0) return -1;
        if (n > LIMIT_MAX_TASKS) return -1;
        r->u.nb_task = n;
        r->u.task_ids = calloc(n, sizeof(uint64_t));
        if (!r->u.task_ids) return -1;
        for (uint32_t i = 0; i < n; ++i) {
            if (decode_uint64(fd, &r->u.task_ids[i]) < 0) {
                free(r->u.task_ids);
                return -1;
            }
        }
        return 0;
    }
    return -1;
}*/

/* --------------------- answers (daemon -> client) ---------------------
   We implement functions matching each response format described in protocole.md.
---------------------------------------------------------------------*/

/* Helper: encode an ERROR response: ANSTYPE='ER' (uint16) + ERRCODE (uint16) */
int encode_answer_err(int fd, uint16_t errcode)
{
    if (encode_uint16(fd, (uint16_t)ERR) < 0) return -1;
    return encode_uint16(fd, errcode);
}

/* Helper: decode ERROR response */
int decode_answer_err(int fd, uint16_t *errcode_out)
{
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    if (anstype != (uint16_t)ERR) return -1;
    if (decode_uint16(fd, errcode_out) < 0) return -1;
    return 0;
}

/* CREATE / COMBINE OK: ANSTYPE='OK' + TASKID(uint64) */
int encode_answer_ok_taskid(int fd, uint64_t taskid)
{
    if (encode_uint16(fd, (uint16_t)OK) < 0) return -1;
    return encode_uint64(fd, taskid);
}
int decode_answer_ok_taskid(int fd, uint64_t *taskid_out)
{
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    if (anstype != (uint16_t)OK) return -1;
    if (decode_uint64(fd, taskid_out) < 0) return -1;
    return 0;
}

/* REMOVE OK and TERMINATE OK are just ANSTYPE='OK' (no payload) */
int encode_answer_ok_nopayload(int fd)
{
    return encode_uint16(fd, (uint16_t)OK);
}
int decode_answer_ok_nopayload(int fd)
{
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    return (anstype == (uint16_t)OK) ? 0 : -1;
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

int decode_a_list(int fd, a_list_t *ans)
{
    if (!ans) return -1;
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    if (anstype != (uint16_t)OK) return -1;
    if (decode_uint32(fd, &ans->nbtask) < 0) return -1;
    if (ans->nbtask > 0) {
        ans->all_task = calloc(ans->nbtask, sizeof(task_t));
        if (!ans->all_task) return -1;
        for (uint32_t i = 0; i < ans->nbtask; ++i) {
            if (decode_uint64(fd, &ans->all_task[i].id) < 0) { free(ans->all_task); return -1; }
            ans->all_task[i].timing = malloc(sizeof(timing_t));
            if (!ans->all_task[i].timing) { free(ans->all_task); return -1; }
            if (decode_timing(fd, ans->all_task[i].timing) < 0) { free(ans->all_task[i].timing); free(ans->all_task); return -1; }
            /* Commandline is a string -> decode into temporary string_t */
            string_t tmp;
            if (decode_string(fd, &tmp) < 0) { free(ans->all_task[i].timing); free(ans->all_task); return -1; }
            /* Here we don't reconstruct command_t; store commandline into task structure if you have a field for it.
               For now we can keep it in a temporary buffer (user code should free tmp.data). */
            /* Example: if task_t has field 'char *commandline', set it: */
            ans->all_task[i].commandline = tmp.data; /* NOTE: requires task_t has commandline field (char*) */
            ans->all_task[i].commandline_len = tmp.length; /* optional */
        }
    } else {
        ans->all_task = NULL;
    }
    return 0;
}

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

int decode_a_output(int fd, a_output_t *ans)
{
    if (!ans) return -1;
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    if (anstype == (uint16_t)ERR) {
        uint16_t ec;
        if (decode_uint16(fd, &ec) < 0) return -1;
        ans->anstype = (uint16_t)ERR;
        ans->errcode = ec;
        ans->output.length = 0;
        ans->output.data = NULL;
        return 0;
    } else if (anstype == (uint16_t)OK) {
        ans->anstype = (uint16_t)OK;
        if (decode_string(fd, &ans->output) < 0) return -1;
        ans->errcode = 0;
        return 0;
    }
    return -1;
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

int decode_a_timecode(int fd, a_timecode_t *a)
{
    if (!a) return -1;
    uint16_t anstype;
    if (decode_uint16(fd, &anstype) < 0) return -1;
    if (anstype == (uint16_t)ERR) {
        uint16_t ec;
        if (decode_uint16(fd, &ec) < 0) return -1;
        a->anstype = (uint16_t)ERR;
        a->errcode = ec;
        a->nbrun = 0;
        a->all_timecode = NULL;
        return 0;
    } else if (anstype == (uint16_t)OK) {
        a->anstype = (uint16_t)OK;
        if (decode_uint32(fd, &a->nbrun) < 0) return -1;
        if (a->nbrun > LIMIT_MAX_RUNS) return -1;
        if (a->nbrun > 0) {
            a->all_timecode = calloc(a->nbrun, sizeof(time_exitcode_t));
            if (!a->all_timecode) return -1;
            for (uint32_t i = 0; i < a->nbrun; ++i) {
                if (decode_int64(fd, &a->all_timecode[i].time) < 0) { free(a->all_timecode); return -1; }
                uint16_t ec;
                if (decode_uint16(fd, &ec) < 0) { free(a->all_timecode); return -1; }
                a->all_timecode[i].exitcode = (int32_t)ec;
            }
        } else {
            a->all_timecode = NULL;
        }
        return 0;
    }
    return -1;
}

/* --------------------------- end of file --------------------------- */
