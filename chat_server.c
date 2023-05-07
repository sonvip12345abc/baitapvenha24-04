#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int server_fd, new_socket, activity, valread, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE], client_names[MAX_CLIENTS][BUFFER_SIZE];
    fd_set readfds;

    // Initialize client_sockets array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific IP and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5000);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections and add them to the client_sockets array
    int addrlen = sizeof(address);

    while (1) {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        // If there is a new connection, add it to the client_sockets array
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Process incoming messages from the clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                valread = read(sd,buffer,BUFFER_SIZE);
                if (valread == 0) {
                // Client disconnected, remove it from the client_sockets array
                close(sd);
                client_sockets[i] = 0;
                memset(client_names[i], 0, sizeof(client_names[i]));
            }
            else {
                // Process incoming message
                if (strlen(client_names[i]) == 0) {
                    // Ask client for their name if it hasn't been set yet
                    send(sd, "What is your name?\n", strlen("What is your name?\n"), 0);
                    memset(buffer, 0, sizeof(buffer));
                    valread = read(sd, buffer, BUFFER_SIZE);

                    if (valread > 0) {
                        buffer[valread] = '\0';

                        // Check if the message has the correct format
                        char *pos = strstr(buffer, ": ");
                        if (pos != NULL) {
                            pos[0] = '\0';

                            // Save the client's name
                            strcpy(client_names[i], buffer);
                            printf("New client connected: %s\n", client_names[i]);
                        }
                        else {
                            send(sd, "Invalid format. Please try again.\n", strlen("Invalid format. Please try again.\n"), 0);
                            continue;
                        }
                    }
                    else {
                        // Error reading from client socket
                        perror("read error");
                        continue;
                    }
                }
                else {
                    // Broadcast message to all other clients
                    char message[BUFFER_SIZE + 32];
                    sprintf(message, "%s: %s", client_names[i], buffer);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != 0 && client_sockets[j] != sd) {
                            send(client_sockets[j], message, strlen(message), 0);
                        }
                    }
                }
            }
        }
    }
}

return 0;
}