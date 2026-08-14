#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUF 1024

int server_socket;
char latency_filename[64];
int test_duration;

/* ===== TIME FUNCTION ===== */

long long current_time_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

/* ===== LATENCY LOG ===== */

void log_latency(long long latency)
{
    FILE *fp = fopen(latency_filename, "a");
    if (!fp)
        return;

    fprintf(fp, "%lld\n", latency);
    fclose(fp);
}

/* ===== FRAMING ===== */

int send_msg1(int s, const char *msg)
{
    uint32_t len = htonl(strlen(msg));
    send(s, &len, 4, 0);
    send(s, msg, strlen(msg), 0);
    return 0;
}

int recv_msg1(int s, char *buf)
{
    uint32_t len;
    if (recv(s, &len, 4, MSG_WAITALL) <= 0)
        return -1;

    len = ntohl(len);
    if (len >= BUF)
        return -1;

    if (recv(s, buf, len, MSG_WAITALL) <= 0)
        return -1;

    buf[len] = 0;
    return len;
}

/* ===== RECEIVE THREAD ===== */

void *receive_thread(void *arg)
{
    char buf[BUF];

    while (1)
    {

        if (recv_msg1(server_socket, buf) <= 0)
            return NULL;

        char copy[BUF];
        strcpy(copy, buf);

        char *type = strtok(copy, "|");

        if (type && strcmp(type, "MESSAGE") == 0)
        {

            strtok(NULL, "|"); // sender
            char *timestamp_str = strtok(NULL, "|");
            strtok(NULL, ""); // message

            if (timestamp_str)
            {
                long long send_time = atoll(timestamp_str);
                long long recv_time = current_time_microseconds();
                long long latency = recv_time - send_time;

                log_latency(latency);
            }
        }
    }
}

/* ===== MAIN ===== */

int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        printf("Usage: ./loadtest <fork|thread|select> <username> <duration_sec>\n");
        return 1;
    }

    char *server_type = argv[1];
    char *username = argv[2];
    test_duration = atoi(argv[3]);

    snprintf(latency_filename, sizeof(latency_filename),
             "%s_latency.txt", server_type);

    printf("Latency logging → %s\n", latency_filename);

    /* Connect to server */

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Connect failed");
        return 1;
    }

    /* Send username */
    send_msg1(server_socket, username);

    char buf[BUF];
    recv_msg1(server_socket, buf); // CONNECTED

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, receive_thread, NULL);

    /* Send messages automatically */

    long long start_time = current_time_microseconds();

    while (1)
    {

        long long now = current_time_microseconds();
        if ((now - start_time) > (long long)test_duration * 1000000LL)
            break;

        long long ts = current_time_microseconds();

        char msg[BUF];
        snprintf(msg, sizeof(msg),
                 "BROADCAST|%lld|hello_from_%s",
                 ts, username);

        send_msg1(server_socket, msg);
        // sleep(1);
        usleep(100000); // send 1 msg per second
    }

    close(server_socket);
    return 0;
}