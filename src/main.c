#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <uv.h>
#include <sodium.h>

#include <guid.h>
#include <message.h>
#include <settings.h>
#include <peer_list.h>
#include <message_history.h>
#include <network.h>
void on_sigint(uv_signal_t *handle, int signum) {
    uv_stop(loop);
    uv_signal_stop(handle);
    printf("\rinfo: shutting down\n");
}

int main(int argc, char **argv) {
   
    parse_arguments(argc, argv);

    /* create a libuv event loop to manage asynchronous events */
    loop = uv_default_loop();

    /* 
     * Register signal handler for SIGINT. When the user manually kills the
     * node, we gracefully stop the libuv event loop so that the program can
     * exit normally, allowing the builtin clang memory sanitizer to check
     * for memory leaks.
     * 
     * Note that we do not use the standard POSIX signal handling mechanism
     * for compatability with windows. The libuv signal handling mechanism
     * emulates SIGINT whenever the users presses CTRL+C.
     */
    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_sigint, SIGINT);

    /*
     * Attempt to establish a peer-to-peer connection for each of the
     * connections specified in the command line arguments. 
     * 
     * Note that nearly all nodes should attempt to connect to another node
     * when joining the network. The only exception is the 'bootstrap' node
     * that other nodes first connect to when joining the network.
     */
    for (int i = 0; i < settings.n_peer_connections; i++) {
        char *addr = settings.peer_addresses[i];
        int port = settings.peer_ports[i];
        int err = connect_to_peer(addr, port);
        if (err) {
            printf("error: unable to connect to peer %s:%d\n", addr, port);
        }
    }
 
    /*
     * Listen on the specified port for incoming peer connections. 
     * 
     * Note that not every node will necessarily accept incoming connections.
     * If specified, a node can act as a "client-only" node that connects and
     * sends messages to other nodes, but does not allow nodes to initiate a
     * connection.
     */
    if (settings.should_listen) {
        int err = listen_for_peer_connections(settings.port, settings.backlog);
        if (err != 0) {
            printf("error: unable to listen on port %d: %s\n", settings.port, uv_err_name(err));
        } else {
            printf("info: accepting connections on port %d\n", settings.port);
        }
    }

    /*
     * Start the libuv event loop. This will take complete control over
     * program flow until uv_stop is called.
     */
    uv_run(loop, UV_RUN_DEFAULT);
}