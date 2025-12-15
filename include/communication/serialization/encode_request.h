#include "communication/request.h"

int encode_simple_request(int fd, const simple_request_t *req);

/* ============================================================
 * COMPLEX REQUEST (CR, CB)
 * CR: OPCODE + TIMING + ARGUMENTS
 * CB: OPCODE + TIMING + TYPE(uint16)
 *          + NBTASKS(uint32) + TASKID[]
 * ============================================================ */
int encode_complex_request(int fd, const complex_request_t *req);