#ifndef SETTINGS_H
#define SETTINGS_H

#define DEFAULT_PORT 1960
#define DEFAULT_SHOULD_LISTEN 1
#define DEFAULT_BACKLOG 128
#define MAX_INITIAL_CONNECTIONS 64

struct settings_t {
    int port;
    int backlog;
    int should_listen;
    char peer_addresses[MAX_INITIAL_CONNECTIONS][16];
    int peer_ports[MAX_INITIAL_CONNECTIONS];
    int n_peer_connections;
};

extern struct settings_t settings;

void parse_arguments(int argc, char **argv);

#endif /* SETTINGS_H */