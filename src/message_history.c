#include <message_history.h>

guid_t message_history[MESSAGE_HISTORY_SIZE] = {0};
int message_history_back = 0;

void message_history_add(guid_t guid) {
    message_history[message_history_back] = guid;
    message_history_back = (message_history_back + 1) % MESSAGE_HISTORY_SIZE;
}

int message_history_has(guid_t guid) {
    for (int i = 0; i < MESSAGE_HISTORY_SIZE; i++) {
        if (guid_compare(guid, message_history[i]) == 0) return 1;
    }
    return 0;
}
