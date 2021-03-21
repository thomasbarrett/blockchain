#ifndef PEER_LIST_H
#define PEER_LIST_H

#include <uv.h>

struct peer_list_t {
    uv_tcp_t *peer;
    struct peer_list_t *prev;
    struct peer_list_t *next;
};

extern struct peer_list_t *peers;

void add_node_to_peer_list(struct peer_list_t *node);
void remove_node_from_peer_list(struct peer_list_t *node);

#endif /* PEER_LIST_H */