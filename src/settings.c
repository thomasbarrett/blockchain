#include <string.h>
#include <stdio.h>

#include <settings.h>

struct settings_t settings;

/**
 * Parse command line arguments into the global settings struct. 
 * 
 * The accepted arguments are as follows:
 * --port=<value>               Set listening port
 * --connect=<address>:<port>   Add address:port to initial connection list
 * --backlog=<value>            Set server backlog size
 */
void parse_arguments(int argc, char **argv) {
    
    settings.port = DEFAULT_PORT;
    settings.backlog = DEFAULT_BACKLOG;
    settings.should_listen = DEFAULT_SHOULD_LISTEN;
    
    /*
     * Runtime settings determined from a combination of defaults and command
     * line argumnets. The current allowed arguments are...
     * 
     * port: the port to listen for connections.
     * bootstrap: should the node act as a bootstrap node.
     */
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            
            int *port = &settings.port;
            int *backlog = &settings.backlog;
            int *should_listen = &settings.should_listen;
            char *peer_address = (char *) &settings.peer_addresses[settings.n_peer_connections];
            int *peer_port = (int *) &settings.peer_ports[settings.n_peer_connections];

            if (sscanf(argv[i], "-port=%d", port) == 1) continue;
            if (sscanf(argv[i], "-backlog=%d", backlog) == 1) continue;
            if (sscanf(argv[i], "-should-listen=%d", should_listen) == 1) continue;
            
            /* allow up to MAX_INITIAL_CONNECTIONS --connect arguments */
            if (settings.n_peer_connections < MAX_INITIAL_CONNECTIONS) {
                if (sscanf(argv[i], "-connect=%15[^':']:%d", peer_address, peer_port) == 2) {
                    settings.n_peer_connections += 1;
                    continue;
                }
            }

            printf("error: unknown argument: %s\n", argv[i]);
        }
    }
}