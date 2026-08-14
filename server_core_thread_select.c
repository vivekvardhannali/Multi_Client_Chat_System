#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server_core.h"
#include "history.h"   // 🔥 ADDED
#include "monitor.h"
#define MAX_CLIENTS 100
#define BUF 1024

struct Client {
    int socket_fd;
    char username[50];
    int active;
};

struct Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ================= CLIENT MANAGEMENT ================= */

int add_active_user(int socket, char *username) {

    pthread_mutex_lock(&clients_mutex);

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active &&
           strcmp(clients[i].username,username)==0){
            pthread_mutex_unlock(&clients_mutex);
            return 0;
        }
    }

    for(int i=0;i<MAX_CLIENTS;i++){
        if(!clients[i].active){
            clients[i].socket_fd = socket;
            strcpy(clients[i].username,username);
            clients[i].active = 1;
            pthread_mutex_unlock(&clients_mutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return 0;
}

void remove_active_user(int socket){

    pthread_mutex_lock(&clients_mutex);

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active &&
           clients[i].socket_fd==socket){
            clients[i].active = 0;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

int find_client_by_socket(int socket){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active &&
           clients[i].socket_fd==socket)
            return i;
    }
    return -1;
}

int find_client_by_username(char *username){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active &&
           strcmp(clients[i].username,username)==0)
            return clients[i].socket_fd;
    }
    return -1;
}

/* ================= MESSAGE HANDLING ================= */

void broadcast_message(int sender_fd,char *message){

    pthread_mutex_lock(&clients_mutex);

    int idx = find_client_by_socket(sender_fd);
    if(idx==-1){
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    char full[BUF];
    snprintf(full,sizeof(full),
             "MESSAGE|%s|%s",
             clients[idx].username,
             message);

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active &&
           clients[i].socket_fd!=sender_fd){
            send_msg(clients[i].socket_fd,full);
        }
    }

    /* 🔥 LOG MESSAGE */
    log_message(
        clients[idx].username,
        "ALL",
        "BROADCAST",
        message
    );

    pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(int sender_fd,
                          char *receiver,
                          char *message){

    pthread_mutex_lock(&clients_mutex);

    int rfd = find_client_by_username(receiver);
    int idx = find_client_by_socket(sender_fd);

    if(rfd==-1 || idx==-1){
        pthread_mutex_unlock(&clients_mutex);
        send_msg(sender_fd,"USER_NOT_ONLINE");
        return;
    }

    char full[BUF];
    snprintf(full,sizeof(full),
             "PRIVATE|%s|%s",
             clients[idx].username,
             message);

    send_msg(rfd,full);

    /* 🔥 LOG MESSAGE */
    log_message(
        clients[idx].username,
        receiver,
        "PRIVATE",
        message
    );

    pthread_mutex_unlock(&clients_mutex);
}

void send_active_user_list(int client_fd){

    pthread_mutex_lock(&clients_mutex);

    char list[BUF]="ONLINE|";

    for(int i=0;i<MAX_CLIENTS;i++){
        if(clients[i].active){
            strcat(list,clients[i].username);
            strcat(list,",");
        }
    }

    int len = strlen(list);
    if(len > 0 && list[len-1] == ','){
        list[len-1] = '\0';
    }

    pthread_mutex_unlock(&clients_mutex);
    send_msg(client_fd,list);
}

void handle_client(int client_fd){

    char buffer[BUF];

    if(recv_msg(client_fd,buffer)<=0){
        close(client_fd);
        return;
    }

    if(!add_active_user(client_fd,buffer)){
        send_msg(client_fd,"USERNAME_ALREADY_IN_USE");
        close(client_fd);
        return;
    }

    send_msg(client_fd,"CONNECTED");

    while(1){

        if(recv_msg(client_fd,buffer)<=0)
            break;

        if(process_message(client_fd,buffer))
            break;
    }

    remove_active_user(client_fd);
    close(client_fd);
}

int process_message(int client_fd, char *buffer){

    char *command = strtok(buffer,"|");
    if(!command) return 0;

    if(strcmp(command,"BROADCAST")==0){
        char *msg = strtok(NULL,"");
        if(msg) broadcast_message(client_fd,msg);
    }
    else if(strcmp(command,"PRIVATE")==0){
        char *user = strtok(NULL,"|");
        char *msg = strtok(NULL,"");
        if(user && msg)
            send_private_message(client_fd,user,msg);
    }
    else if(strcmp(command,"LIST")==0){
        send_active_user_list(client_fd);
    }
    else if(strcmp(command,"HISTORY")==0){

        int idx = find_client_by_socket(client_fd);
        if(idx != -1){
            send_user_history(
                client_fd,
                clients[idx].username
            );
        }
    }
    else if(strcmp(command,"EXIT")==0){
        return 1;
    }

    return 0;
}

/* ================= FRAMING ================= */

int send_msg(int s,const char *msg){
    uint32_t len = htonl(strlen(msg));
    if(send(s,&len,4,0)<=0) return -1;
    if(send(s,msg,strlen(msg),0)<=0) return -1;
    return 0;
}

int recv_msg(int s,char *buf){
    uint32_t len;
    if(recv(s,&len,4,MSG_WAITALL)<=0) return -1;
    len = ntohl(len);
    if(len>=BUF) return -1;
    if(recv(s,buf,len,MSG_WAITALL)<=0) return -1;
    buf[len]='\0';
    return len;
}

void register_with_discovery(const char *disc_ip,
                             int disc_port,
                             const char *my_ip,
                             int my_port)
{
    int s = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(disc_port);
    inet_pton(AF_INET,disc_ip,&addr.sin_addr);

    if(connect(s,(struct sockaddr*)&addr,sizeof(addr))<0){
        close(s);
        return;
    }

    char msg[256];
    snprintf(msg,sizeof(msg),
             "REGISTER_CHAT_SERVER %s %d",
             my_ip,my_port);

    send_msg(s,msg);
    close(s);
}