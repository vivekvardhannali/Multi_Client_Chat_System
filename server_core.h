#ifndef SERVER_CORE_H
#define SERVER_CORE_H

/* Client management */
int add_active_user(int socket, char *username);
void remove_active_user(int socket);
int find_client_by_username(char *username);
int find_client_by_socket(int socket);
void init_shared_memory();
/* Messaging */
void handle_client(int client_fd);   // <-- IMPORTANT
int process_message(int client_fd, char *buffer);
void broadcast_message(int sender_fd, char *message);
void send_private_message(int sender_fd,
                          char *receiver,
                          char *message);
void send_active_user_list(int client_fd);

/* Framing */
int send_msg(int s, const char *msg);
int recv_msg(int s, char *buf);

/* Discovery */
void register_with_discovery(const char *disc_ip,
                             int disc_port,
                             const char *my_ip,
                             int my_port);

#endif