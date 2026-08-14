#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server_core.h"
#include "monitor.h"
#define CHAT_PORT 5000
#define DISCOVERY_IP "127.0.0.1"
#define DISCOVERY_PORT 9090

void *client_thread(void *arg) {

    int client_fd = *(int *)arg;
    free(arg);

    handle_client(client_fd);

    return NULL;
}

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

    printf("Threaded Chat Server running on port %d...\n", CHAT_PORT);

    /* Register with discovery */
    register_with_discovery(DISCOVERY_IP,
                            DISCOVERY_PORT,
                            "127.0.0.1",
                            CHAT_PORT);
    start_monitoring("thread_metrics.txt");
    while (1) {

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
            continue;

        pthread_t tid;

        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;

        if (pthread_create(&tid, NULL,
                           client_thread,
                           pclient) != 0) {

            perror("Thread creation failed");
            close(client_fd);
            free(pclient);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}