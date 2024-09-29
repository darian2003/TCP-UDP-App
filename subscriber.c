#include "helpers.h"

void printTCP(struct tcp_message msg) {

    char sign = msg.udp_message.payload[0];

    // Data types encodings are specified in the pdf
    if (msg.udp_message.data_type == 0) {
        // INT
        int number_module;
        // Copying the number's module
        memcpy(&number_module, &(msg.udp_message.payload[1]), sizeof(int));
        number_module = ntohl(number_module);
        if (sign) {
        number_module *= -1;
    }

    printf("%s:%d - %s - INT - %d\n", inet_ntoa(msg.udp_client.sin_addr),
           ntohs(msg.udp_client.sin_port), msg.udp_message.topic, number_module);
    } else if (msg.udp_message.data_type == 1) {
        // SHORT REAL
        uint16_t number_module;
        memcpy(&number_module, &(msg.udp_message.payload), sizeof(uint16_t));
        float short_real= (float)ntohs(number_module)/100;
        printf("%s:%d - %s - SHORT_REAL - %f\n",
           inet_ntoa(msg.udp_client.sin_addr), ntohs(msg.udp_client.sin_port), msg.udp_message.topic, short_real);

    }  else if (msg.udp_message.data_type == 2) {
        // FLOAT
        int number_module;
        uint8_t exponent_module;

        memcpy(&number_module, &(msg.udp_message.payload[1]), sizeof(int));
        memcpy(&exponent_module, &(msg.udp_message.payload[5]), 1);

        number_module = ntohl(number_module);

        // separating the fractional part to the integer part
        number_module /= pow(10, exponent_module);
        if (sign) {
            number_module *= -1;
        }
        printf("%s:%d - %s - FLOAT - %lf\n",
           inet_ntoa(msg.udp_client.sin_addr), ntohs(msg.udp_client.sin_port), msg.udp_message.topic, (float)number_module);
    } else if (msg.udp_message.data_type == 3) {
  	    // STRING.

    printf(
        "%s:%d - %s - STRING - %s\n", inet_ntoa(msg.udp_client.sin_addr),
        ntohs(msg.udp_client.sin_port), msg.udp_message.topic, msg.udp_message.payload);
  }
}

int main(int argc, char *argv[]) {

    int ret;
    char buffer[1500];
    struct sockaddr_in client_address, server_address;

    // Checking the correct number of arguments received
    if (argc < 4) {
        printf("Incorrect number of arguments\n");
        return -1;
    }
    
    // Creating client's socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Deactivating nagle's algorithm
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    // Deactivating buffering 
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);  
    setvbuf(stdin, NULL, _IONBF, BUFSIZ);

    // Completing server information
    memset(&server_address, 0, sizeof(struct sockaddr_in));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &server_address.sin_addr);
    DIE(ret == 0, "inet_aton");

    // Connecting to server
    ret = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    DIE(ret < 0, "connect");

    // Sending our id
    ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "send");

    // The server is going to send a message with "close" if 
    // the client with this id has already been connectede to this server before
    // => meaning that a socket with this fd has already been created and is monitored
    // by poll()
    ret = recv(sockfd, buffer, 1500, 0);
    DIE(ret < 0, "recv");
    if (strncmp(buffer, "close", strlen("close")) == 0) {
        // This client has already been connected to the server before
        close(sockfd);
        return 0;
    }

    // The client needs to monitor only the server and the stdin
    struct pollfd pfds[2];
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;


    while (1) {

        // timeout is negative because we want to wait forever
        ret = poll(pfds, 2, -7);
        DIE(ret < 0, "poll");

        struct tcp_message msg;

        // We received data from the server
         if (pfds[0].revents & POLLIN) {
            ret = recv(sockfd, &msg, sizeof(msg), 0);
            DIE(ret < 0, "recv");
            if (ret == 0)  {
                close(sockfd);
                return -1;
            }
            printTCP(msg);

        } else if (pfds[1].revents & POLLIN) {
            // We received data from STDIN
            memset(buffer, 0, 1500);
            fgets(buffer, 1499, stdin);

            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            } else {
                // Process subscription request
                char *command = strtok(buffer, " ");
                if (command == NULL) {
                    printf("Strtok failed 1\n");
                    return -1;
                }
                char *topic_name = strtok(NULL, " \n");
                if (topic_name == NULL) {
                    printf("Strtok failed 2\n");
                    return -1;
                }

                if (strcmp(command, "subscribe") == 0) {
                    subscription msg;
                    memset(&msg, 0, sizeof(msg));
                    strcpy(msg.subscribe, "subscribe");
                    strcpy(msg.topic, topic_name);

                    char *sf = strtok(NULL, "\0\n ");
                    if (!sf) {
                        printf("Strtok failed 3\n"); 
                        return -1;
                    }

                    msg.sf = atoi(sf);

                    ret = send(sockfd, &msg, sizeof(msg), 0);
                    DIE(ret < 0, "send");
                    printf("Subscribed to topic.\n");
                } else if (strcmp(command, "unsubscribe") == 0) {
                    subscription msg;
                    strcpy(msg.subscribe, "unsubscribe");
                    strcpy(msg.topic, topic_name);
                    ret = send(sockfd, &msg, sizeof(msg), 0);
                    DIE(ret < 0, "send");
                    printf("Unsubscribed from topic.\n");
                } else {
                    printf("Unknown command!\n");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}