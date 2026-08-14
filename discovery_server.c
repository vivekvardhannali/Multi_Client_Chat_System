// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <pthread.h>

// #define DISCOVERY_IP "127.0.0.1"
// #define DISCOVERY_PORT 9090
// #define MAX_USERS 100
// #define BUF 1024

// typedef struct {
//     char username[32];
//     char password[32];
// } user;

// user users[MAX_USERS];
// int user_count = 0;

// char chat_ip[INET_ADDRSTRLEN] = "";
// int chat_port = 0;

// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// /* ===== FRAMING ===== */

// int send_msg(int s,const char *msg){
//     uint32_t len=htonl(strlen(msg));
//     if(send(s,&len,4,0)<=0)return -1;
//     if(send(s,msg,strlen(msg),0)<=0)return -1;
//     return 0;
// }

// int recv_msg(int s,char *buf){
//     uint32_t len;
//     if(recv(s,&len,4,MSG_WAITALL)<=0)return -1;
//     len=ntohl(len);
//     if(recv(s,buf,len,MSG_WAITALL)<=0)return -1;
//     buf[len]=0;
//     return len;
// }

// /* ===== FIND USER ===== */

// int find_user(char *username){
//     for(int i=0;i<user_count;i++){
//         if(strcmp(users[i].username,username)==0)
//             return i;
//     }
//     return -1;
// }

// /* ===== THREAD HANDLER ===== */

// /* ===== THREAD HANDLER ===== */

// void* handle(void *arg){

//     int s=*(int*)arg;
//     free(arg);

//     char buf[BUF];

//     while(1){

//         if(recv_msg(s,buf)<=0){
//             close(s);
//             pthread_exit(NULL);
//         }

//         /* CHECK USER */
//         if(!strncmp(buf,"CHECK_USER ",11)){

//             char username[32];
//             sscanf(buf+11,"%s",username);

//             pthread_mutex_lock(&lock);

//             int idx=find_user(username);

//             pthread_mutex_unlock(&lock);

//             if(idx==-1)
//                 send_msg(s,"USER_NOT_FOUND");
//             else
//                 send_msg(s,"USER_EXISTS");
//         }

//         /* VERIFY PASSWORD */
//         else if(!strncmp(buf,"VERIFY ",7)){

//             char u[32],p[32];
//             sscanf(buf+7,"%s %s",u,p);

//             pthread_mutex_lock(&lock);

//             int idx=find_user(u);

//             if(idx==-1){
//                 pthread_mutex_unlock(&lock);
//                 send_msg(s,"USER_NOT_FOUND");
//                 continue;
//             }

//             if(strcmp(users[idx].password,p)!=0){
//                 pthread_mutex_unlock(&lock);
//                 send_msg(s,"WRONG_PASSWORD");
//                 continue;
//             }

//             if(strlen(chat_ip)==0){
//                 pthread_mutex_unlock(&lock);
//                 send_msg(s,"CHAT_SERVER_NOT_AVAILABLE");
//                 continue;
//             }

//             char response[BUF];
//             snprintf(response,sizeof(response),
//                      "LOGIN_SUCCESS %s %d",
//                      chat_ip,chat_port);

//             pthread_mutex_unlock(&lock);
//             send_msg(s,response);
//         }

//         /* SIGNUP */
//         else if(!strncmp(buf,"SIGNUP ",7)){

//             char u[32],p[32];
//             sscanf(buf+7,"%s %s",u,p);

//             pthread_mutex_lock(&lock);

//             if(find_user(u)!=-1){
//                 pthread_mutex_unlock(&lock);
//                 send_msg(s,"USER_ALREADY_EXISTS");
//                 continue;
//             }

//             if(user_count>=MAX_USERS){
//                 pthread_mutex_unlock(&lock);
//                 send_msg(s,"SERVER_FULL");
//                 continue;
//             }

//             strcpy(users[user_count].username,u);
//             strcpy(users[user_count].password,p);
//             user_count++;

//             pthread_mutex_unlock(&lock);

//             send_msg(s,"SIGNUP_SUCCESS");
//         }

//         /* REGISTER CHAT SERVER */
//         else if(!strncmp(buf,"REGISTER_CHAT_SERVER ",21)){

//             char ip[INET_ADDRSTRLEN];
//             int port;

//             sscanf(buf+21,"%s %d",ip,&port);

//             pthread_mutex_lock(&lock);
//             strcpy(chat_ip,ip);
//             chat_port=port;
//             pthread_mutex_unlock(&lock);

//             send_msg(s,"CHAT_SERVER_REGISTERED");
//         }

//         else{
//             send_msg(s,"UNKNOWN_COMMAND");
//         }
//     }
// }

// /* ===== MAIN ===== */

// int main(){

//     int server_fd=socket(AF_INET,SOCK_STREAM,0);

//     int opt=1;
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//     struct sockaddr_in addr;
//     addr.sin_family=AF_INET;
//     addr.sin_port=htons(DISCOVERY_PORT);
//     inet_pton(AF_INET,DISCOVERY_IP,&addr.sin_addr);

//     bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
//     listen(server_fd,10);

//     printf("Discovery running at %s:%d\n",
//            DISCOVERY_IP,DISCOVERY_PORT);

//     while(1){

//         int c=accept(server_fd,NULL,NULL);

//         pthread_t t;
//         int *p=malloc(sizeof(int));
//         *p=c;

//         pthread_create(&t,NULL,handle,p);
//         pthread_detach(t);
//     }
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define DISCOVERY_IP "127.0.0.1"
#define DISCOVERY_PORT 9090
#define MAX_USERS 100
#define BUF 1024

typedef struct {
    char username[32];
    char password[32];
} user;

user users[MAX_USERS];
int user_count = 0;

char chat_ip[INET_ADDRSTRLEN] = "";
int chat_port = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* ===== FRAMING ===== */

int send_msg(int s,const char *msg){
    uint32_t len=htonl(strlen(msg));
    if(send(s,&len,4,0)<=0)return -1;
    if(send(s,msg,strlen(msg),0)<=0)return -1;
    return 0;
}

int recv_msg(int s,char *buf){
    uint32_t len;
    if(recv(s,&len,4,MSG_WAITALL)<=0)return -1;
    len=ntohl(len);
    if(len>=BUF) return -1;
    if(recv(s,buf,len,MSG_WAITALL)<=0)return -1;
    buf[len]=0;
    return len;
}

/* ===== FIND USER ===== */

int find_user(char *username){
    for(int i=0;i<user_count;i++){
        if(strcmp(users[i].username,username)==0)
            return i;
    }
    return -1;
}

/* ===== THREAD HANDLER ===== */

void* handle(void *arg){

    int s=*(int*)arg;
    free(arg);

    char buf[BUF];

    while(1){

        if(recv_msg(s,buf)<=0){
            close(s);
            pthread_exit(NULL);
        }

        /* CHECK USER */
        if(!strncmp(buf,"CHECK_USER ",11)){

            char username[32];
            sscanf(buf+11,"%31s",username);

            pthread_mutex_lock(&lock);
            int idx=find_user(username);
            pthread_mutex_unlock(&lock);

            if(idx==-1)
                send_msg(s,"USER_NOT_FOUND");
            else
                send_msg(s,"USER_EXISTS");
        }

        /* VERIFY */
        else if(!strncmp(buf,"VERIFY ",7)){

            char u[32],p[32];
            sscanf(buf+7,"%31s %31s",u,p);

            pthread_mutex_lock(&lock);
            int idx=find_user(u);

            if(idx==-1){
                pthread_mutex_unlock(&lock);
                send_msg(s,"USER_NOT_FOUND");
                continue;
            }

            if(strcmp(users[idx].password,p)!=0){
                pthread_mutex_unlock(&lock);
                send_msg(s,"WRONG_PASSWORD");
                continue;
            }

            if(strlen(chat_ip)==0){
                pthread_mutex_unlock(&lock);
                send_msg(s,"CHAT_SERVER_NOT_AVAILABLE");
                continue;
            }

            char response[BUF];
            snprintf(response,sizeof(response),
                     "LOGIN_SUCCESS %s %d",
                     chat_ip,chat_port);

            pthread_mutex_unlock(&lock);
            send_msg(s,response);
        }

        /* SIGNUP */
        else if(!strncmp(buf,"SIGNUP ",7)){

            char u[32],p[32];
            sscanf(buf+7,"%31s %31s",u,p);

            pthread_mutex_lock(&lock);

            if(find_user(u)!=-1){
                pthread_mutex_unlock(&lock);
                send_msg(s,"USER_ALREADY_EXISTS");
                continue;
            }

            if(user_count>=MAX_USERS){
                pthread_mutex_unlock(&lock);
                send_msg(s,"SERVER_FULL");
                continue;
            }

            strcpy(users[user_count].username,u);
            strcpy(users[user_count].password,p);
            user_count++;

            pthread_mutex_unlock(&lock);
            send_msg(s,"SIGNUP_SUCCESS");
        }

        /* REGISTER CHAT SERVER */
        else if(!strncmp(buf,"REGISTER_CHAT_SERVER ",21)){

            char ip[INET_ADDRSTRLEN];
            int port;

            sscanf(buf+21,"%15s %d",ip,&port);

            pthread_mutex_lock(&lock);
            strcpy(chat_ip,ip);
            chat_port=port;
            pthread_mutex_unlock(&lock);

            send_msg(s,"CHAT_SERVER_REGISTERED");
        }

        else{
            send_msg(s,"UNKNOWN_COMMAND");
        }
    }
}

/* ===== MAIN ===== */

int main(){
    signal(SIGPIPE, SIG_IGN);
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd < 0){
        perror("Socket failed");
        exit(1);
    }

    int opt=1;
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,
                  &opt,sizeof(opt))<0){
        perror("setsockopt failed");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(DISCOVERY_PORT);
    inet_pton(AF_INET,DISCOVERY_IP,&addr.sin_addr);

    if(bind(server_fd,(struct sockaddr*)&addr,sizeof(addr))<0){
        perror("Bind failed");
        exit(1);
    }

    if(listen(server_fd,10)<0){
        perror("Listen failed");
        exit(1);
    }

    printf("Discovery running at %s:%d\n",
           DISCOVERY_IP,DISCOVERY_PORT);

    while(1){

        int c=accept(server_fd,NULL,NULL);
        if(c<0){
            perror("Accept failed");
            continue;
        }

        pthread_t t;
        int *p=malloc(sizeof(int));
        *p=c;

        if(pthread_create(&t,NULL,handle,p)!=0){
            perror("Thread creation failed");
            close(c);
            free(p);
            continue;
        }

        pthread_detach(t);
    }

    close(server_fd);
    return 0;
}