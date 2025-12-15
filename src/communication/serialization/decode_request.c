#include "communication/request.h"
#include "communication/serialization/serialization.h"
#include "communication/serialization/en_decode_struct.h"

int decode_complex_request(int fd, complex_request_t *r)
{
    if (!r) return -1;
    if (decode_uint16(fd, &r->opcode) < 0) return -1;
    if (decode_timing(fd, &r->timing) < 0) return -1;

    if (r->opcode == CR) {
        if (r->u.command.type == SI) {
            if (encode_arguments(fd, &r->u.command.args.simple) < 0) return -1;
        } else {
            return -1; 
        }
        return 0;
    } else if (r->opcode == CB) {
        if (decode_uint16(fd, &r->u.composed.type) < 0) return -1;
        uint32_t n;
        if (decode_uint32(fd, &n) < 0) return -1;
        if (n > LIMIT_MAX_TASKS) return -1;
        r->u.composed.nb_task = n;
        r->u.composed.task_ids = calloc(n, sizeof(uint64_t));
        if (!r->u.composed.task_ids) return -1;
        for (uint32_t i = 0; i < n; ++i) {
            if (decode_uint64(fd, &r->u.composed.task_ids[i]) < 0) {
                free(r->u.composed.task_ids);
                return -1;
            }
        }
        return 0;
    }
    return -1;
}

/* Simple request: OPCODE(uint16) + optional TASKID(uint64) */
int decode_simple_request(int fd, simple_request_t *r)
{
    if (!r) return -1;
    if (decode_uint16(fd, &r->opcode) < 0) return -1;
    if (r->opcode == LS || r->opcode == TM) return 0;
    if (decode_uint64(fd, &r->task_id) < 0) return -1;
    return 0;
}