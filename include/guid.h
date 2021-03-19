#ifndef GUID_H
#define GUID_H

#include <stdint.h>

/*
 * This represents a guid (globally unique identifier) value
 * as an array of 32 bit unsigned integers. Note that in this
 * representation, the first integer i[0] contains the most significant
 * bytes while the last integer i[3] contains the least significant
 * bytes.
 * 
 * Consider the following guid: 2c1cd0f9-5f98-b1fa-5273-aa0149e8c907
 * 
 * On a big-endian system, it is stored as the following:
 * 2c1cd0f9 5f98b1fa 5273aa01 49e8c907
 * 
 * On a little-endian system, it is stored as the following:
 * f9d01c2c fab1985f 01aa7352 07c9e849
 */
typedef struct {
    uint32_t i[4];
} guid_t;

/*
 * Generates a new guid by generating 4 cryptographically secure
 * pseduorandomly generated numbers. In this case, we use libsodium
 * to provide the random number generator.
 */
guid_t guid_new();

/*
 * Compares the values of two guids. If g1 is greater than g2, this
 * function will return 1. If g1 is less than g2, this function will
 * return -1. If g1 is equal to g2, this function will return 0.
 */
int guid_compare(guid_t g1, guid_t g2);

/*
 * Print the guid to stdout as a 128-bit hexidecimal number. Note that this
 * function does not print the guid in standard guid format with dashes. 
 * This should mainly be used as a utility function for debugging.
 */
void guid_print(guid_t guid);

#endif /* GUID_H */