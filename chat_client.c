#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

#define DISCOVERY_IP "127.0.0.1"
#define DISCOVERY_PORT 9090
#define BUF 1024

int server_socket;
char latency_filename[64];   // 🔥 NEW: global latency file name

/* ===== TIME FUNCTION ===== */

long long current_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
}

/* ===== LATENCY LOGGING ===== */

void log_latency(long long latency) {
    FILE *fp = fopen(latency_filename, "a");
    if (!fp) return;

    fprintf(fp, "%lld\n", latency);
    fclose(fp);
}

/* ===== FRAMING ===== */

int send_msg1(int s,const char *msg){
    uint32_t len=htonl(strlen(msg));

    int sent = 0;
    while(sent < 4){
        int n = send(s, ((char*)&len)+sent, 4-sent, 0);
        if(n <= 0) return -1;
        sent += n;
    }

    sent = 0;
    int msg_len = strlen(msg);
    while(sent < msg_len){
        int n = send(s, msg+sent, msg_len-sent, 0);
        if(n <= 0) return -1;
        sent += n;
    }

    return 0;
}

int recv_msg1(int s,char *buf){
    uint32_t len;
    int received = 0;

    while(received < 4){
        int n = recv(s, ((char*)&len)+received, 4-received, 0);
        if(n <= 0) return -1;
        received += n;
    }

    len = ntohl(len);

    if(len >= BUF)
        return -1;

    received = 0;
    while(received < len){
        int n = recv(s, buf+received, len-received, 0);
        if(n <= 0) return -1;
        received += n;
    }

    buf[len] = 0;
    return len;
}

/* ===== AUTH ===== */

int authenticate(char *username,
                 char *server_ip,
                 int *server_port){

    while(1){

        printf("\nUsername: ");
        scanf("%s",username);

        int s=socket(AF_INET,SOCK_STREAM,0);

        struct sockaddr_in addr;
        addr.sin_family=AF_INET;
        addr.sin_port=htons(DISCOVERY_PORT);
        inet_pton(AF_INET,DISCOVERY_IP,&addr.sin_addr);

        connect(s,(struct sockaddr*)&addr,sizeof(addr));

        char msg[BUF],buf[BUF];

        snprintf(msg,sizeof(msg),
                 "CHECK_USER %s",username);

        send_msg1(s,msg);
        recv_msg1(s,buf);

        if(strcmp(buf,"USER_NOT_FOUND")==0){

            printf("User not found. Create account? (y/n): ");
            char ch;
            scanf(" %c",&ch);

            if(ch!='y' && ch!='Y'){
                close(s);
                continue;
            }

            char password[32];
            printf("Enter new password: ");
            scanf("%s",password);

            snprintf(msg,sizeof(msg),
                     "SIGNUP %s %s",
                     username,password);

            send_msg1(s,msg);
            recv_msg1(s,buf);

            printf("%s\n",buf);
            close(s);
            continue;
        }

        else if(strcmp(buf,"USER_EXISTS")==0){

            char password[32];
            printf("Password: ");
            scanf("%s",password);

            snprintf(msg,sizeof(msg),
                     "VERIFY %s %s",
                     username,password);

            send_msg1(s,msg);
            recv_msg1(s,buf);

            if(strncmp(buf,"LOGIN_SUCCESS ",14)==0){
                sscanf(buf+14,"%s %d",
                       server_ip,server_port);
                close(s);
                return 0;
            }

            printf("%s\n",buf);
            close(s);
        }

        else{
            printf("%s\n",buf);
            close(s);
        }
    }
}

/* ===== RECEIVE THREAD ===== */

void* receive_thread(void *arg){
    char buf[BUF];

    while(1){

        if(recv_msg1(server_socket,buf)<=0){
            printf("\nDisconnected from server.\n");
            exit(0);
        }

        char copy[BUF];
        strcpy(copy,buf);

        char *type = strtok(copy,"|");

        if(type && strcmp(type,"MESSAGE")==0){

            char *sender = strtok(NULL,"|");
            char *timestamp_str = strtok(NULL,"|");
            char *message = strtok(NULL,"");

            if(sender && timestamp_str && message){

                long long send_time = atoll(timestamp_str);
                long long recv_time = current_time_microseconds();
                long long latency = recv_time - send_time;

                printf("\n%s: %s\n> ", sender, message);
                fflush(stdout);

                log_latency(latency);
            }
        }
        else if(type && strcmp(type,"PRIVATE")==0){

            char *sender = strtok(NULL,"|");
            char *timestamp_str = strtok(NULL,"|");
            char *message = strtok(NULL,"");

            if(sender && timestamp_str && message){

                long long send_time = atoll(timestamp_str);
                long long recv_time = current_time_microseconds();
                long long latency = recv_time - send_time;

                printf("\n[PRIVATE] %s: %s\n> ", sender, message);
                fflush(stdout);

                log_latency(latency);
            }
        }
        else{
            printf("\n%s\n> ",buf);
            fflush(stdout);
        }
    }
}

/* ===== MAIN ===== */

int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: ./client <fork|thread|select>\n");
        return 1;
    }

    /* 🔥 Set latency file automatically */
    snprintf(latency_filename, sizeof(latency_filename),
             "%s_latency.txt", argv[1]);

    printf("Logging latency to: %s\n", latency_filename);

    char username[32];
    char server_ip[INET_ADDRSTRLEN];
    int server_port;

    if(authenticate(username,
                    server_ip,
                    &server_port)<0){
        printf("Authentication failed\n");
        return 1;
    }

    server_socket=socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(server_port);
    inet_pton(AF_INET,server_ip,&addr.sin_addr);

    connect(server_socket,
            (struct sockaddr*)&addr,
            sizeof(addr));

    send_msg1(server_socket,username);

    char buf[BUF];
    recv_msg1(server_socket,buf);
    printf("%s\n",buf);

    pthread_t t;
    pthread_create(&t,NULL,receive_thread,NULL);

    getchar();

    char input[BUF];

    while(1){

        printf("> ");
        fgets(input,sizeof(input),stdin);
        input[strcspn(input,"\n")]=0;

        if(strncmp(input,"BROADCAST|",10)==0){

            char *msg = input + 10;
            long long ts = current_time_microseconds();

            char new_msg[BUF];
            snprintf(new_msg,sizeof(new_msg),
                     "BROADCAST|%lld|%s",
                     ts,msg);

            send_msg1(server_socket,new_msg);
        }
        else if(strncmp(input,"PRIVATE|",8)==0){

            char temp[BUF];
            strcpy(temp,input);

            char *cmd = strtok(temp,"|");
            char *user = strtok(NULL,"|");
            char *msg  = strtok(NULL,"");

            if(user && msg){
                long long ts = current_time_microseconds();

                char new_msg[BUF];
                snprintf(new_msg,sizeof(new_msg),
                         "PRIVATE|%s|%lld|%s",
                         user,ts,msg);

                send_msg1(server_socket,new_msg);
            }
        }
        else{
            send_msg1(server_socket,input);
        }

        if(!strncmp(input,"EXIT",4))
            break;
    }

    close(server_socket);
    return 0;
}