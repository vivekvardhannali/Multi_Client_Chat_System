#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include "history.h"
#include "monitor.h"
#define PORT 5000
#define MAX_CLIENTS 100
#define BUF 1024

typedef struct {
    int sock;
    int ipc_fd;
    char username[32];
    int active;
} Client;

Client clients[MAX_CLIENTS];

typedef struct {
    int sender_index;
    char msg[BUF];
} IpcMsg;

/* ================= FRAMING ================= */

int send_msg(int s,const char *msg){
    uint32_t len=htonl(strlen(msg));
    if(send(s,&len,4,0)<=0) return -1;
    if(send(s,msg,strlen(msg),0)<=0) return -1;
    return 0;
}

int recv_msg(int s,char *buf){
    uint32_t len;
    if(recv(s,&len,4,MSG_WAITALL)<=0) return -1;
    len=ntohl(len);
    if(len>=BUF) return -1;
    if(recv(s,buf,len,MSG_WAITALL)<=0) return -1;
    buf[len]='\0';
    return len;
}

/* ================= DISCOVERY ================= */

void register_with_discovery(){

    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if(connect(s,(struct sockaddr*)&addr,sizeof(addr))<0){
        perror("Discovery connect failed");
        return;
    }

    char msg[128];
    snprintf(msg,sizeof(msg),
             "REGISTER_CHAT_SERVER 127.0.0.1 %d", PORT);

    send_msg(s,msg);

    printf("Registered with discovery\n");
    close(s);
}

/* ================= CLIENT MGMT ================= */

int add_client(int sock,int ipc_fd,char *username){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(!clients[i].active){
            clients[i].active=1;
            clients[i].sock=sock;
            clients[i].ipc_fd=ipc_fd;
            strcpy(clients[i].username,username);
            return i;
        }
    }
    return -1;
}

void remove_client(int idx){
    close(clients[idx].sock);
    close(clients[idx].ipc_fd);
    clients[idx].active=0;
}

/* ================= COMMAND HANDLING ================= */

void handle_command(int sender_idx,char *buffer){

    char *cmd=strtok(buffer,"|");
    if(!cmd) return;

    /* -------- BROADCAST -------- */
    if(strcmp(cmd,"BROADCAST")==0){

        char *msg=strtok(NULL,"");
        if(!msg) return;

        char out[BUF];
        snprintf(out,sizeof(out),
                 "MESSAGE|%s|%s",
                 clients[sender_idx].username,msg);

        for(int i=0;i<MAX_CLIENTS;i++){
            if(clients[i].active && i!=sender_idx){
                send_msg(clients[i].sock,out);
            }
        }

        log_message(
            clients[sender_idx].username,
            "ALL",
            "BROADCAST",
            msg
        );
    }

    /* -------- PRIVATE -------- */
    else if(strcmp(cmd,"PRIVATE")==0){

        char *user=strtok(NULL,"|");
        char *msg=strtok(NULL,"");
        if(!user||!msg) return;

        for(int i=0;i<MAX_CLIENTS;i++){
            if(clients[i].active &&
               strcmp(clients[i].username,user)==0){

                char out[BUF];
                snprintf(out,sizeof(out),
                         "PRIVATE|%s|%s",
                         clients[sender_idx].username,msg);

                send_msg(clients[i].sock,out);

                log_message(
                    clients[sender_idx].username,
                    user,
                    "PRIVATE",
                    msg
                );

                return;
            }
        }

        send_msg(clients[sender_idx].sock,"USER_NOT_ONLINE");
    }

    /* -------- LIST -------- */
    else if(strcmp(cmd,"LIST")==0){

        char out[BUF]="ONLINE|";
        int first = 1;

        for(int i=0;i<MAX_CLIENTS;i++){
            if(clients[i].active){
                if(!first) strcat(out,",");
                strcat(out,clients[i].username);
                first = 0;
            }
        }

        send_msg(clients[sender_idx].sock,out);
    }

    /* -------- HISTORY -------- */
    else if(strcmp(cmd,"HISTORY")==0){

        send_user_history(
            clients[sender_idx].sock,
            clients[sender_idx].username
        );
    }

    /* -------- EXIT -------- */
    else if(strcmp(cmd,"EXIT")==0){
        remove_client(sender_idx);
    }
}

/* ================= MAIN ================= */

int main(){

    int server_fd=socket(AF_INET,SOCK_STREAM,0);

    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(PORT);

    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,10);

    printf("Fork+IPC Chat Server running on port %d\n",PORT);

    register_with_discovery();
    start_monitoring("fork_metrics.txt");
    while(1){

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd,&readfds);

        int maxfd=server_fd;

        for(int i=0;i<MAX_CLIENTS;i++){
            if(clients[i].active){
                FD_SET(clients[i].ipc_fd,&readfds);
                if(clients[i].ipc_fd>maxfd)
                    maxfd=clients[i].ipc_fd;
            }
        }

        select(maxfd+1,&readfds,NULL,NULL,NULL);

        /* ===== NEW CONNECTION ===== */
        if(FD_ISSET(server_fd,&readfds)){

            int new_sock=accept(server_fd,NULL,NULL);

            char username[32];
            if(recv_msg(new_sock,username)<=0){
                close(new_sock);
                continue;
            }

            int sp[2];
            socketpair(AF_UNIX,SOCK_STREAM,0,sp);

            int idx=add_client(new_sock,sp[0],username);
            if(idx<0){
                send_msg(new_sock,"SERVER_FULL");
                close(new_sock);
                close(sp[0]); close(sp[1]);
                continue;
            }

            send_msg(new_sock,"CONNECTED");

            pid_t pid=fork();

            if(pid==0){
                close(server_fd);
                close(sp[0]);

                while(1){
                    char buf[BUF];
                    if(recv_msg(new_sock,buf)<=0) break;

                    IpcMsg m;
                    m.sender_index=idx;
                    strcpy(m.msg,buf);

                    write(sp[1],&m,sizeof(m));
                }

                close(sp[1]);
                close(new_sock);
                exit(0);
            }

            close(sp[1]);
        }

        /* ===== IPC MESSAGES ===== */
        for(int i=0;i<MAX_CLIENTS;i++){
            if(clients[i].active &&
               FD_ISSET(clients[i].ipc_fd,&readfds)){

                IpcMsg m;
                int r=read(clients[i].ipc_fd,&m,sizeof(m));

                if(r<=0){
                    remove_client(i);
                    continue;
                }

                handle_command(m.sender_index,m.msg);
            }
        }

        while(waitpid(-1,NULL,WNOHANG)>0){}
    }

    return 0;
}