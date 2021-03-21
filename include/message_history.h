#ifndef MESSAGE_HISTORY_H
#define MESSAGE_HISTORY_H

#include <guid.h>

#define MESSAGE_HISTORY_SIZE 1024

void message_history_add(guid_t guid);
int message_history_has(guid_t guid);


#endif /* MESSAGE_HISTORY_H */