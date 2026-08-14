#ifndef HISTORY_H
#define HISTORY_H

void log_message(const char *sender,
                 const char *receiver,
                 const char *type,
                 const char *message);

void send_user_history(int client_fd,
                       const char *username);

#endif