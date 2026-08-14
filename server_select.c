/* ONLY CHANGE: remove mutex dependency issues */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "server_core.h"
#include "monitor.h"
#define CHAT_PORT 5000
#define DISCOVERY_IP "127.0.0.1"
#define DISCOVERY_PORT 9090
#define MAX_CLIENTS 100
#define BUF 1024

int main() {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CHAT_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Select Chat Server running on port %d...\n", CHAT_PORT);

    register_with_discovery(DISCOVERY_IP,
                            DISCOVERY_PORT,
                            "127.0.0.1",
                            CHAT_PORT);
    start_monitoring("select_metrics.txt");
    int client_fds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++)
        client_fds[i] = -1;

    fd_set readfds;

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > max_fd)
                    max_fd = client_fds[i];
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0)
            continue;

        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = new_fd;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {

            int fd = client_fds[i];
            if (fd == -1) continue;

            if (FD_ISSET(fd, &readfds)) {

                char buffer[BUF];

                if (recv_msg(fd, buffer) <= 0) {
                    remove_active_user(fd);
                    close(fd);
                    client_fds[i] = -1;
                    continue;
                }

                if (find_client_by_socket(fd) == -1) {
                    if (!add_active_user(fd, buffer)) {
                        send_msg(fd, "USERNAME_ALREADY_IN_USE");
                        close(fd);
                        client_fds[i] = -1;
                    } else {
                        send_msg(fd, "CONNECTED");
                    }
                } else {
                    if (process_message(fd, buffer)) {
                        remove_active_user(fd);
                        close(fd);
                        client_fds[i] = -1;
                    }
                }
            }
        }
    }

    return 0;
}