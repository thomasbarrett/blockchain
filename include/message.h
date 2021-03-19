#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <uv.h>
#include <guid.h>

#define MESSAGE_MAGIC_NUMBER 0x54524a54
#define MESSAGE_BASIC 1
#define MESSAGE_HEADER_SIZE (2 * sizeof(uint32_t) + sizeof(guid_t) + sizeof(uint16_t))

#endif /* MESSAGE_H */