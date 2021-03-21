#include <peer_list.h>

struct peer_list_t *peers = NULL;

void add_node_to_peer_list(struct peer_list_t *node) {
    if (peers == NULL) {
        peers = node;
    } else {
        node->next = peers;
        peers->prev = node;
        peers = node;
    }
}

void remove_node_from_peer_list(struct peer_list_t *node) {
    if (node == peers) {
        peers = node->next;
        if (node->next) node->next->prev = node->prev;
    } else {
        node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
    }
}