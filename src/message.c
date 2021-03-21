#include <uv.h>

#include <message.h>

guid_t message_get_guid(char *message) {
    guid_t result;
    for (int j = 0; j < 4; j++) {
        result.i[j] = ntohl(((uint32_t *) message)[2 + j]);
    }
    return result;
}

void message_set_guid(char *message, guid_t guid) {
    for (int j = 0; j < 4; j++) {
        ((uint32_t *) message)[2 + j] = htonl(guid.i[j]);
    }
}

uint32_t message_get_length(char *message) {
    return ntohl(((uint32_t *) message)[1]);
}

void message_set_length(char *message, uint32_t length) {
    ((uint32_t *) message)[1] = htonl(length);
}

uint32_t message_get_magic(char *message) {
    return ntohl(((uint32_t *) message)[0]);
}

void message_set_magic(char *message, uint32_t magic) {
    ((uint32_t *) message)[0] = htonl(magic);
}

uint16_t message_get_type(char *message) {
    return *((uint16_t *)(message + 24));
}

void message_set_type(char *message, uint16_t type) {
    *((uint16_t *)(message + 24)) = htons(type);
}

