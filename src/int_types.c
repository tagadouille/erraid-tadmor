#include "int_types.h"
#include "io_utils.h"
//#include <endian.h> 
#include <string.h> 

/** Macro to handle the generic logic for reading an integer type.
 */
#define READ_INT_GENERIC(T, CONV) \
    int read_##T(int fd, T *value) { \
        T network_value; \
        if (read_all(fd, &network_value, sizeof(T)) == -1) { \
            return -1; \
        } \
        *value = CONV(network_value); \
        return 0; \
    }

/** Macro to handle the generic logic for writing an integer type.
 */
#define WRITE_INT_GENERIC(T, CONV) \
    int write_##T(int fd, T value) { \
        T network_value = CONV(value); \
        if (write_all(fd, &network_value, sizeof(T)) == -1) { \
            return -1; \
        } \
        return 0; \
    }

/* --- Non-signed Integers Implementation --- */

READ_INT_GENERIC(uint16_t, be16toh)
WRITE_INT_GENERIC(uint16_t, htobe16)

READ_INT_GENERIC(uint32_t, be32toh)
WRITE_INT_GENERIC(uint32_t, htobe32)

READ_INT_GENERIC(uint64_t, be64toh)
WRITE_INT_GENERIC(uint64_t, htobe64)

/* --- Signed Integers Implementation --- */

READ_INT_GENERIC(int16_t, be16toh)
WRITE_INT_GENERIC(int16_t, htobe16)

READ_INT_GENERIC(int32_t, be32toh)
WRITE_INT_GENERIC(int32_t, htobe32)

READ_INT_GENERIC(int64_t, be64toh)
WRITE_INT_GENERIC(int64_t, htobe64)
