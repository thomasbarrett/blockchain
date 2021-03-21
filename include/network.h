#ifndef NETWORK_H
#define NETWORK_H

extern uv_loop_t *loop;

int connect_to_peer(char *address, int port);
int listen_for_peer_connections(int port, int backlog);

#endif /* NETWORK_H */