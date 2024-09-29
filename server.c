#include "helpers.h"
#include <poll.h>
#include <math.h>
#include <unistd.h>

void printClean(struct tcp_client client) {
    printf("Client %s %d\n", client.id, client.status);
    for (int i = 0; i < client.topic_arr_size; i++) 
        printf("%s %d\n", client.topics[i].name, client.topics[i].sf);
}

struct tcp_client create_client(char *id, int sockfd) {

    struct tcp_client *new_client = (struct tcp_client *)calloc(1, sizeof(struct tcp_client));
    DIE(new_client == NULL, "calloc client");

    memset(new_client->id, 0, 11);
    memcpy(new_client->id, id, strlen(id));

    new_client->sockfd = sockfd;
    new_client->status = ACTIVE;
    new_client->queued_messages = queue_create();

    return *new_client;
}

// shifts every element from pfds starting from "socket_index" position to the right
// resulting in deleting the socket at socket_index position
void delete_socket(struct pollfd *pfds, int pfds_size, int socket_index) {
    for (int j = socket_index; j < pfds_size- 1; j++) {
        pfds[j] = pfds[j + 1];
    }
}

void reconnect_client(struct tcp_client *client, int sockfd) {

    client->status = ACTIVE;
    client->sockfd = sockfd;

    // If SF = 1, send the client all messages that were sent on this topic
    // while this client was offline
    while (!queue_empty(client->queued_messages)) {
        struct tcp_message *msg = (struct tcp_message *)queue_deq(client->queued_messages);
        int ret = send(client->sockfd, msg, sizeof(tcp_message), 0);
        DIE(ret < 0, "send");
        free(msg);
    }
}

int main(int argc, char **argv) {

    // Checking for correct number of parameters
    if (argc != 2) {
        printf("\n Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    int ret;
    struct sockaddr_in server_address, client_address;
    socklen_t client_length = sizeof(client_address);
    struct tcp_client tcp_clients[MAX_CLIENTS];
    int tcp_clients_no = 0;
    char buffer[1500];
    memset(buffer, 0, 1500);

    // Deactivating buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    setvbuf(stdin, NULL, _IONBF, BUFSIZ);

    uint16_t server_port = atoi(argv[1]);
     DIE(server_port == 0, "atoi");

    // create a socket for UDP clients
    int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sockfd < 0, "socket udp_sockfd");

    // create a socket for TCP clients
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sockfd < 0, "socket tcp_sockfd");

    // Deactivating Nagle's algorithm
    int enable = 1;
    if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    memset(&server_address, 0 , sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // set-up passive listening socket for TCP clients
    ret = bind(tcp_sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in));
    DIE(ret < 0, "bind failed");
    ret = listen(tcp_sockfd, MAX_CLIENTS);
    DIE(ret < 0, "listen failed");

    ret = bind(udp_sockfd, (struct sockaddr_in *)&server_address, sizeof(struct sockaddr_in));
    DIE(ret < 0, "bind failed");

 /*
struct pollfd {
        int fd;                 // the socket descriptor
        short events;     // bitmap of events we're interested in
        short revents;    // when poll() returns, bitmap of events that occurred
};
*/ 

// Using poll() for Synchronous I/O Multiplexing
/* poll() is horribly slow when it comes to giant numbers of connections.
    Given the fact that the maximum number of connections that this socket enables is 100, poll() is going
    to be our best option */

    // Setting events to POLLIN since we are awaiting data/commands from clients
    struct pollfd pfds[MAX_CLIENTS];
    pfds[0].fd = tcp_sockfd;
    pfds[0].events = POLLIN;
    pfds[1].fd = udp_sockfd;
    pfds[1].events = POLLIN;
    pfds[2].fd = STDIN_FILENO;
    pfds[2].events = POLLIN;

    int poll_arr_size = 3;

    while (1) {

        // Passing poll() a negativ timeout -> we want the wait to never end
        ret = poll(pfds, poll_arr_size, -1);
        DIE(ret < 0 , "poll error");

        /* Check all the monitored file descriptors in order to find out which one
            is ready to be read */
        for (int i = 0; i < poll_arr_size; i++) {
            if (pfds[i].revents & POLLIN) {
                if (pfds[i].fd == STDIN_FILENO) {
                    // STANDARD INPUT
                    memset(buffer, 0, sizeof(buffer));
                    fgets(buffer, sizeof(buffer), stdin);

                    // We are only interested in receiving "exit"
                    if (strncmp(buffer, "exit", 4) == 0) {
                        for (i = 0; i <= tcp_clients_no; ++i) 
                            close(pfds[i].fd);
                    }
                } else if (pfds[i].fd == udp_sockfd) {
                    // UDP
                    struct udp_message udp_msg;
                    struct tcp_message tcp_msg;
                    memset(&udp_msg, 0, sizeof(struct udp_message));
                    memset(&client_address, 0, client_length);
                    ret = recvfrom(udp_sockfd, &udp_msg, sizeof(struct udp_message), 0, (struct sockaddr_in *)&client_address, &client_length);
                    DIE(ret < 0, "recvfrom error");

                    memset(&tcp_msg, 0, sizeof(tcp_message));
                    memcpy(&(tcp_msg.udp_client), &client_address, client_length);
                    memcpy(&(tcp_msg.udp_message), &udp_msg, sizeof(udp_message));

                    // Send this message to all the clients that are subscribed to this topic
                    for (int i = 0; i < tcp_clients_no; i++) {
                        for (int j = 0; j < tcp_clients[i].topic_arr_size; j++) {
                            struct tcp_client client = tcp_clients[i];
                            if (strcmp(client.topics[j].name, udp_msg.topic) == 0) {
                                // check if active
                                if (client.status == ACTIVE) {
                                    ret = send(client.sockfd, &tcp_msg, sizeof(tcp_msg), 0);
                                    DIE(ret < 0, "failed to send message");
                                } else if (client.topics[j].sf == SF) {
                                    tcp_message *save_the_msg = (tcp_message *)calloc(1, sizeof(tcp_message));
                                    DIE(save_the_msg == NULL, "calloc");

                                    memcpy(save_the_msg, &tcp_msg, sizeof(tcp_message));
                                    queue_enq(client.queued_messages, save_the_msg);
                                }
                            }
                        }
                    } 
                } else if (pfds[i].fd == tcp_sockfd) {
                    // A TCP connection request came in
                    // Check if the server port has a free connection available
                    // (should always have an available conection since MAX_CLIENTS is set to 100)
                    DIE(poll_arr_size >= MAX_CLIENTS, "Maximum number of clients reached");
                    struct sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);

                    // The file descriptor given to accept() is the TCP socket set to listen
                    int new_client_sockfd = accept(tcp_sockfd, (struct sockaddr_in*)&client_address,
                                    (socklen_t*)&client_len);
                    DIE(new_client_sockfd < 0, "accept failed");

                    char client_id[11];

                    memset(client_id, 0, sizeof(client_id));
                    ret = recv(new_client_sockfd, client_id, sizeof(client_id), 0);
                    DIE(ret < 0, "recv");

                    // Now we need to check whether a client with this ID has already been
                    // added to the server before. If so, then we need to close this newly created
                    // socket because we already have an individual socket for this particular client.
                    // Right now, the client is blocked waiting for the server's response.
                    // The server will send "close" if this id has already been used and the socket
                    // needs to be closed, or "ok" if everything is fine

                    int client_found = 0;
                    int j = 0;
                    for (j = 0; j < tcp_clients_no; j++) {
                        if (strcmp(tcp_clients[j].id, client_id) == 0) {
                            client_found = 1;
                            break;
                        }
                    }
                    if (client_found && tcp_clients[j].status == ACTIVE) {
                        printf("Client %s already connected.\n", client_id);  
                        close(new_client_sockfd);     
                    } else if (client_found == 0 || tcp_clients[j].status == INACTIVE) {
                        
                        // Add client to poll array
                        memset(&pfds[poll_arr_size], 0, sizeof(struct pollfd));
                        pfds[poll_arr_size].fd = new_client_sockfd;
                        pfds[poll_arr_size].events = POLLIN;
                        poll_arr_size++;

                        // Let the client know that the connection has been established
                        // Also unblock the blocking recv() call that the client made
                        strcpy(buffer, "ok");
                        ret = send(new_client_sockfd, buffer, sizeof(buffer), 0);
                        DIE(ret < 0, "send");

                        // Print the according message
                        printf("New client %s connected from %s:%i.\n", client_id,
                                inet_ntoa(client_address.sin_addr), client_address.sin_port);

                        // If this client is new, add it to the tcp_clients array list
                        if (client_found == 0) {
                            tcp_clients[tcp_clients_no] = create_client(client_id, new_client_sockfd);
                            tcp_clients_no++;
                        } else {
                            // This client needs to be reconnected
                            reconnect_client(&tcp_clients[j], new_client_sockfd);
                        }
                    }

                } else {
                    // Data has been received on one of the sockets opened
                    // for an individual TCP client
                    // The server received a subscrption command from a client
                    struct subscription msg;
                    memset(&msg, 0, sizeof(msg));
                    ret = recv(pfds[i].fd, &msg, sizeof(msg), 0);
                    DIE(ret < 0, "recv");

                    if (ret == 0) {
                        // connection has been closed
                        for (int k = 0; k < tcp_clients_no; k++) {
                            if (tcp_clients[k].sockfd == pfds[i].fd) {
                                tcp_clients[k].status = INACTIVE;
                                printf("Client %s disconnected.\n", tcp_clients[k].id);
                                close(pfds[i].fd);
                                delete_socket(pfds, poll_arr_size, i);
                                poll_arr_size--;
                                i--;
                                break;
                            }
                        }
                    } else {
                        // process subscribe or unsubscribe
                        if (strncmp(msg.subscribe, "subscribe", strlen("subscribe")) == 0) {
                            for (int j = 0; j < tcp_clients_no; j++) {
                                struct tcp_client *client = &tcp_clients[j];
                                if (client->sockfd == pfds[i].fd) {
                                    int topic_exists = 0;
                                    for (int k = 0; k < client->topic_arr_size; k++) {
                                        if (strcmp(client->topics[k].name, msg.topic) == 0) {
                                            topic_exists = 1;
                                            break;
                                        }
                                    }
                                    if (topic_exists) {
                                        printf("Client %s already subscribed.\n", client->id);
                                    } else {
                                        // add this topic to the topic array list of this client
                                        strcpy(client->topics[client->topic_arr_size].name, msg.topic);
                                        client->topics[client->topic_arr_size].sf = msg.sf;
                                        client->topic_arr_size++; 
                                    }
                                }
                            }
                        } else if (strncmp(msg.subscribe, "unsubscribe", strlen("unsubscribe")) == 0) {
                            // delete the topic from the client's topics array
                            for (int j = 0; j < tcp_clients_no; j++) {
                                struct tcp_client *client = &tcp_clients[j];
                                if (client->sockfd == pfds[i].fd) {
                                    int topic_exists = 0;
                                    for (int k = 0; k < client->topic_arr_size; k++) {
                                        if (strcmp(client->topics[k].name, msg.topic) == 0) {
                                            // delete the topic from client's subscribed topics array
                                            topic_exists = 1;
                                            for (int t = k; t < tcp_clients[j].topic_arr_size-1; t++) {
                                                tcp_clients[j].topics[t] = tcp_clients[t].topics[t+1];
                                            }
                                            tcp_clients[j].topic_arr_size--;
                                            break;
                                        } 
                                    }
                                }
                            }
                        }
                    }
                } 
            }
        }

    }

    close(tcp_sockfd);
    close(udp_sockfd);
    return 0;
}