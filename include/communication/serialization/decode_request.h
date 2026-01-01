#include "communication/request.h"

/* ============================================================
 * SIMPLE REQUEST (LS, RM, SO, SE, TX, TM)
 * OPCODE(uint16) + TASKID(optional uint64)
 * ============================================================ */

int decode_simple_request(int fd, simple_request_t *req);

int decode_complex_request(int fd, complex_request_t *req);