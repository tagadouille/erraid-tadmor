#include "communication/request.h"
#include "communication/serialization/serialization.h"

/* Complex request:
   - CR: OPCODE + TIMING + COMMAND (arguments)
   - CB: OPCODE + TIMING + TYPE(uint16) + NBTASKS(uint32) + TASKID[...]
*/
int encode_complex_request(int fd, const complex_request_t *r)
{
    if (!r) return -1;
    if (encode_uint16(fd, r->opcode) < 0) return -1;
    if (encode_timing(fd, &r->timing) < 0) return -1;

    if (r->opcode == CR) {
        if (r->u.command.type == SI) {
            if (encode_arguments(fd, &r->u.command.args.simple) < 0) return -1;
        } else {
            // gérer les commandes composées si nécessaire
            return -1; // ou autre logique
        }
        return 0;
    } else if (r->opcode == CB) {
        if (encode_uint16(fd, r->u.composed.type) < 0) return -1;
        if (r->u.composed.nb_task > LIMIT_MAX_TASKS) return -1;
        if (encode_uint32(fd, r->u.composed.nb_task) < 0) return -1;
        for (uint32_t i = 0; i < r->u.composed.nb_task; ++i) {
            if (encode_uint64(fd, r->u.composed.task_ids[i]) < 0) return -1;
        }
        return 0;
    }
    return -1;
}

int encode_simple_request(int fd, const simple_request_t *r)
{
    if (!r) return -1;
    if (encode_uint16(fd, r->opcode) < 0) return -1;
    /* LS and TM have no task id; other opcodes have task id */
    if (r->opcode == LS || r->opcode == TM) return 0;
    return encode_uint64(fd, r->task_id);
}