#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "history.h"

#define HISTORY_FILE "chat_history.json"
#define MAX_OBJECT_SIZE 4096

extern int send_msg(int s, const char *msg);

/* ================= LOG MESSAGE ================= */

void log_message(const char *sender,
                 const char *receiver,
                 const char *type,
                 const char *message)
{
    int fd = open(HISTORY_FILE, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return;

    flock(fd, LOCK_EX);

    off_t size = lseek(fd, 0, SEEK_END);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S", t);

    if (size == 0) {
        dprintf(fd,
            "[\n"
            "  {\n"
            "    \"timestamp\": \"%s\",\n"
            "    \"sender\": \"%s\",\n"
            "    \"receiver\": \"%s\",\n"
            "    \"type\": \"%s\",\n"
            "    \"message\": \"%s\"\n"
            "  }\n"
            "]",
            timestamp, sender, receiver, type, message);
    } else {
        lseek(fd, -1, SEEK_END);

        dprintf(fd,
            ",\n"
            "  {\n"
            "    \"timestamp\": \"%s\",\n"
            "    \"sender\": \"%s\",\n"
            "    \"receiver\": \"%s\",\n"
            "    \"type\": \"%s\",\n"
            "    \"message\": \"%s\"\n"
            "  }\n"
            "]",
            timestamp, sender, receiver, type, message);
    }

    flock(fd, LOCK_UN);
    close(fd);
}

/* ================= FILTERED HISTORY ================= */

void send_user_history(int client_fd,
                       const char *username)
{
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (!fp) {
        send_msg(client_fd, "[]");
        return;
    }

    char line[1024];
    char object[MAX_OBJECT_SIZE];
    int inside_object = 0;
    int obj_len = 0;

    char *response = malloc(65536);
    if (!response) {
        fclose(fp);
        send_msg(client_fd, "[]");
        return;
    }

    strcpy(response, "[\n");
    int first = 1;

    while (fgets(line, sizeof(line), fp)) {

        if (strchr(line, '{')) {
            inside_object = 1;
            obj_len = 0;
            memset(object, 0, sizeof(object));
        }

        if (inside_object) {
            int l = strlen(line);
            if (obj_len + l < MAX_OBJECT_SIZE) {
                strcpy(object + obj_len, line);
                obj_len += l;
            }
        }

        if (strchr(line, '}')) {
            inside_object = 0;

            /* 🔍 Check if object belongs to this user */
            int belongs = 0;

            char sender_pattern[128];
            char receiver_pattern[128];

            snprintf(sender_pattern, sizeof(sender_pattern),
                     "\"sender\": \"%s\"", username);

            snprintf(receiver_pattern, sizeof(receiver_pattern),
                     "\"receiver\": \"%s\"", username);

            if (strstr(object, sender_pattern) ||
                strstr(object, receiver_pattern) ||
                strstr(object, "\"receiver\": \"ALL\"")) {
                belongs = 1;
            }

            if (belongs) {
                if (!first)
                    strcat(response, ",\n");

                strcat(response, "  ");
                strcat(response, object);

                first = 0;
            }
        }
    }

    strcat(response, "\n]");
    fclose(fp);

    send_msg(client_fd, response);

    free(response);
}