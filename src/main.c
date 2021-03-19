#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <uv.h>
#include <sodium.h>

#include <guid.h>
#include <message.h>

#define DEFAULT_PORT 1960
#define MESSAGE_HISTORY_SIZE 1024
#define MAX_INITIAL_CONNECTIONS 64

void on_connect(uv_connect_t* connection, int status);

guid_t msg_hist[MESSAGE_HISTORY_SIZE] = {0};
int msg_hist_i = 0;

void msg_hist_add(guid_t guid) {
    msg_hist[msg_hist_i] = guid;
    msg_hist_i = (msg_hist_i + 1) % MESSAGE_HISTORY_SIZE;
}



int msg_hist_has(guid_t guid) {
    for (int i = 0; i < MESSAGE_HISTORY_SIZE; i++) {
        if (guid_compare(guid, msg_hist[i]) == 0) return 1;
    }
    return 0;
}

static uv_loop_t *loop;
static uv_tcp_t server;
static int port = DEFAULT_PORT;

struct peer_list_t {
    uv_tcp_t *peer;
    struct peer_list_t *prev;
    struct peer_list_t *next;
};

struct peer_list_t *peers = NULL;

struct connection_t {
    struct peer_list_t *node;
    char *text;
    size_t text_len;
};

struct write_req_t {
  uv_write_t req;
  uv_buf_t buf;
};

void on_sigint(uv_signal_t *handle, int signum) {
    uv_stop(loop);
    uv_signal_stop(handle);
    printf("\rinfo: shutting down\n");
}

/* Allocate buffers as requested by UV */
static void alloc_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
    char *base = (char *) calloc(1, size);
    if (!base) {
        *buf = uv_buf_init(NULL, 0);
    } else {
        *buf = uv_buf_init(base, size);
    }
}

/* Callback to free the handle */
static void on_close_free(uv_handle_t *handle) {
    free(handle);
}

/* Callback for freeing up all resources allocated for request    */
static void close_data(struct connection_t *data) {
    if (!data) return;
    if (data->node) {
        
        struct sockaddr_in addr;
        uv_tcp_getpeername(data->node->peer, (struct sockaddr *) &addr, &(int){sizeof(addr)});
        printf("[-] %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    
        struct peer_list_t *node = data->node;
        uv_close((uv_handle_t *)node->peer, on_close_free);
        if (node == peers) {
            peers = node->next;
            if (node->next) node->next->prev = node->prev;
        } else {
            node->prev->next = node->next;
            if (node->next) node->next->prev = node->prev;
        }
        free(node);
    }
    if (data->text) free(data->text);
    free(data);
}

/**
 * Connect asynchronously to the peer with the specified address and port.
 * Once a connection has been established, or failed to be established, the
 * on_connect function will be called. 
 * 
 * @param address the ipv4 address of the peer
 * @param port the port of the peer's server
 */
void connect_to_peer(char *address, int port) {
    /* allocate socket for connecting to peer */
    uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    /* connect asynchronously to node with specified address and port */
    struct sockaddr_in *peer_addr = calloc(1, sizeof(struct sockaddr_in));
    uv_ip4_addr(address, port, peer_addr);
    uv_connect_t *connect = malloc(sizeof(uv_connect_t));
    connect->data = peer_addr;
    int err = uv_tcp_connect(connect, socket, (const struct sockaddr*)peer_addr, on_connect);
    if (err) {
        printf("unable to connect: %d\n", err);
        free(peer_addr);
    }
}

void on_write(uv_write_t* wreq, int status) {
    if (status) fprintf(stderr, "uv_write error: %s\n", uv_err_name(status));
    struct write_req_t *req = (struct write_req_t *) wreq;
    free(req->buf.base);
    free(req);
}

static void broadcast_message(char *message, int len) {
    struct peer_list_t *current_node = peers;
    while (current_node != NULL) {
        char *message_copy = malloc(len);
        memcpy(message_copy, message, len);
        struct write_req_t *req = (struct write_req_t*) malloc(sizeof(struct write_req_t));
        req->buf = uv_buf_init(message_copy, len);
        uv_write((uv_write_t*) req, (uv_stream_t*) current_node->peer, &req->buf, 1, on_write);
        current_node = current_node->next;
    }
}

static guid_t message_get_guid(char *message) {
    guid_t result;
    for (int j = 0; j < 4; j++) {
        result.i[j] = ntohl(((uint32_t *) message)[2 + j]);
    }
    return result;
}

static void message_set_guid(char *message, guid_t guid) {
    for (int j = 0; j < 4; j++) {
        ((uint32_t *) message)[2 + j] = htonl(guid.i[j]);
    }
}

static uint32_t message_get_length(char *message) {
    return ntohl(((uint32_t *) message)[1]);
}

static void message_set_length(char *message, uint32_t length) {
    ((uint32_t *) message)[1] = htonl(length);
}

static uint32_t message_get_magic(char *message) {
    return ntohl(((uint32_t *) message)[0]);
}

static void message_set_magic(char *message, uint32_t magic) {
    ((uint32_t *) message)[0] = htonl(magic);
}

static uint16_t message_get_type(char *message) {
    return *((uint16_t *)(message + 24));
}

static void message_set_type(char *message, uint16_t type) {
    *((uint16_t *)(message + 24)) = htons(type);
}

static void handle_message(uv_tcp_t *peer, char *message, int len) {
    guid_t guid = message_get_guid(message);
    uint32_t length = message_get_length(message);
    uint16_t type = message_get_type(message);

    if (!msg_hist_has(guid)) {
        /*
        switch (type) {
            case MESSAGE_BASIC: on_message_basic(message);
        }
        */

        printf("message: %.*s\n", length, message + MESSAGE_HEADER_SIZE); 
        msg_hist_add(guid);
        broadcast_message(message, len);
    }
}

/* 
 * Callback for read function. For large messages, the data may be split into
 * multiple calls to this function. Also, a single call to this function may
 * contain multiple messages. Thus, at every call to this function, we simply
 * concatenate newly read data to the end of a large string, then process this
 * string to parse out messages.
 * 
 * Note that this function owns the dynamically allocated buffer passed in as
 * as the "buf" argument and is responsible for freeing its memory under all
 * circumstances.
 */
static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    
    uv_tcp_t *client = (uv_tcp_t *)stream;
    struct connection_t *data = stream->data;

    /*
     * If an error occured while reading, we should cleanup any allocated
     * memory and close the connection.
     */
    if (nread < 0) {
        free(buf->base);
        close_data(data);
        return;
    } 

    /*
     * Stores data stream as dynamically sized string.
     * Every time the server recieves more data from the stream,
     * it appends the data to the string and updates the size.
     * 
     * We concatenate a '\0' character to the end of the string
     * so that we can use string processing functions such as strstr provided
     * by the standard library.
     */
    if (!data->text) {
        data->text = malloc(nread + 1);
        memcpy(data->text, buf->base, nread);
        data->text_len = nread;
        data->text[data->text_len] = '\0';
    } else {
        data->text = realloc(data->text, data->text_len + nread + 1);
        memcpy(data->text + data->text_len, buf->base, nread);
        data->text_len += nread;
        data->text[data->text_len] = '\0';
    }

    free(buf->base);

    /*
     * We will iterate over all messages currently stored in the buffer and
     * handle them one by one.
     */
    char *text_end = data->text + data->text_len;
    char *message_start = data->text;
    while (message_start + 2 * sizeof(uint32_t) <= text_end) {
        
        /* parse fields from message header */
        uint32_t magic = message_get_magic(message_start);
        uint32_t payload_len = message_get_length(message_start);
        uint32_t message_len = MESSAGE_HEADER_SIZE + payload_len;

        /* 
         * If the first field is not the magic number, skip ahead until the
         * start of the next message is found.
         */
        if (magic != MESSAGE_MAGIC_NUMBER) {
            message_start += sizeof(uint32_t);
            continue;
        }

        /*
         * If the entirety of the current message is not contained in the buffer,
         * then we should wait for more data to process the message.
         */
        if (message_start + message_len > text_end) {
            break;
        }

        handle_message(client, message_start, message_len);
        message_start += message_len;
    }

    /*
     * Copy all unprocessed bytes into a newly allocated buffer
     * and free all processed data.
     */
    if (message_start != data->text) {
        int remainder_len = text_end - message_start;
        char *remainder = (char *) malloc(remainder_len);
        memcpy(remainder, message_start, remainder_len);
        free(data->text);
        data->text = remainder;
        data->text_len = remainder_len;
    }
}

/* Callback for handling the new connection */
static void connection_cb(uv_stream_t *server, int status) {
    if (status != 0) {
        printf("error: unable to handle connection\n");
        return;
    }

    /*
     * Initialize connection data structure and accept connection.
     * The only field which currently has to be initialized is the client
     * field. The other two fields are initialized to NULL and 0 respectively
     * by the calloc function.
     */
    uv_tcp_t *client = (uv_tcp_t *) calloc(1, sizeof(uv_tcp_t));
    struct peer_list_t *node = (struct peer_list_t *) calloc(1, sizeof(struct peer_list_t));
    node->peer = client;
    if (peers == NULL) {
        peers = node;
    } else {
        node->next = peers;
        peers->prev = node;
        peers = node;
    }

    struct connection_t *data;
    data = calloc(1, sizeof(*data));
    data->node = node;
    client->data = data;
    
    /* initialize the new client */
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        struct sockaddr_in addr;
        uv_tcp_getpeername(client, (struct sockaddr *) &addr, &(int){sizeof(addr)});
        printf("[+] %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        uv_read_start((uv_stream_t *)client, alloc_cb, read_cb);
    } else {
        /* close client stream on error */
        printf("connection error\n");
        close_data(data);
    }
}

void on_connect(uv_connect_t* connection, int status) {
    if (status < 0) {
        struct sockaddr_in *addr = connection->data;
        printf("unable to connect: %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        free(connection->data);
        free(connection);
        return;

    } else {
        uv_tcp_t* client = (uv_tcp_t *) connection->handle;
        struct peer_list_t *node = (struct peer_list_t *) calloc(1, sizeof(struct peer_list_t));
        node->peer = client;
        if (peers == NULL) {
            peers = node;
        } else {
            node->next = peers;
            peers->prev = node;
            peers = node;
        }
        struct connection_t *data;
        data = calloc(1, sizeof(*data));
        data->node = node;
        client->data = data;

        free(connection->data);
        free(connection);

        struct sockaddr_in addr;
        uv_tcp_getpeername(client, (struct sockaddr *) &addr, &(int){sizeof(addr)});
        printf("[+] %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        
        char payload[] = "Hello!";
        char message[MESSAGE_HEADER_SIZE + sizeof(payload)];
        guid_t guid = guid_new();
        message_set_magic((char *) &message, MESSAGE_MAGIC_NUMBER);
        message_set_length((char *) &message, sizeof(payload));
        message_set_guid((char *) &message, guid);
        message_set_type((char *) &message, MESSAGE_BASIC);
        memcpy((char *) &message + MESSAGE_HEADER_SIZE, payload, sizeof(payload));

        msg_hist_add(guid);
        broadcast_message(message, MESSAGE_HEADER_SIZE + sizeof(payload));
   
        uv_read_start((uv_stream_t *)client, alloc_cb, read_cb);
    }
}

int main(int argc, char **argv) {
    
    char addresses[MAX_INITIAL_CONNECTIONS][16] = {0};
    int ports[MAX_INITIAL_CONNECTIONS] = {0};
    int n_connections = 0;

    /*
     * Runtime settings determined from a combination of defaults and command
     * line argumnets. The current allowed arguments are...
     * 
     * port: the port to listen for connections.
     * bootstrap: should the node act as a bootstrap node.
     */
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (sscanf(argv[i], "-port=%d", &port) == 1) continue;
            if (sscanf(argv[i], "-connect=%15[^':']:%d", (char *) &addresses[n_connections], (int *) &ports[n_connections]) == 2) {
                n_connections += 1;
                continue;
            }
        }
    }

    loop = uv_default_loop();

    /* 
     * Register signal handler for SIGINT. When the user manually kills the
     * node, we gracefully stop the libuv event loop so that the program can
     * exit normally, allowing the builtin clang memory sanitizer to check
     * for memory leaks.
     */
    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_sigint, SIGINT);

    /*
     * For each of the connections specified in the command line arguments, 
     * attempt to establish a peer-to-peer connection.
     */
    for (int i = 0; i < n_connections; i++) {
        connect_to_peer(addresses[i], ports[i]);
    }

    /* 
     * Initialize socket to listen for incoming connections on the port
     * specified in the command line arguments. If no port is specified,
     * listen on the default port defined above.
     */ 
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_init(loop, &server);
    uv_tcp_bind(&server, (struct sockaddr *)&addr, 0);
    int err = uv_listen((uv_stream_t*) &server, 128, connection_cb);
    if (err != 0) {
        printf("error: unable to start server: %s\n", uv_err_name(err));
    } else {
        printf("info: accepting connections on port %d\n", port);
    }
 
    /*
     * Start the libuv event loop. This will take complete control over
     * program flow until uv_stop is called.
     */
    uv_run(loop, UV_RUN_DEFAULT);
}