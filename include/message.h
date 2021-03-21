#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <guid.h>

#define MESSAGE_MAGIC_NUMBER 0x54524a54
#define MESSAGE_BASIC 1
#define MESSAGE_HEADER_SIZE (2 * sizeof(uint32_t) + sizeof(guid_t) + sizeof(uint16_t))

guid_t message_get_guid(char *message);
uint32_t message_get_length(char *message);
uint32_t message_get_magic(char *message);
uint16_t message_get_type(char *message);

void message_set_guid(char *message, guid_t guid);
void message_set_length(char *message, uint32_t length);
void message_set_magic(char *message, uint32_t magic);
void message_set_type(char *message, uint16_t type);

#endif /* MESSAGE_H */