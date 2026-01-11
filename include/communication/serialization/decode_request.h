#include "communication/request.h"

/**
 * @brief Decode a request from a file descriptor, determining if it's simple or complex.
 * @param fd The file descriptor to read from.
 * @param r Pointer to the request structure to populate.
 * @return 1 or 2 on success, -1 on failure. 
 * If it returns 1 the request is simple, if 2 complex.
 */
int decode_request(int fd, void *r);