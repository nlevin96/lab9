#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "common.h"
#include "line_parser.h"
#include "limits.h"
#define TRUE 1
#define FALSE 0
#define LS_RESP_SIZE 2048

client_state state;
char host_name [2000];
int debug = 0;
int client_id_num = 1;

void print_debug_msg(char* msg){
    if(debug){
        fprintf(stderr, "%s | Log: %s\n", state.server_addr, msg);
    }
}

void init_state(){
    gethostname(host_name, 2000);
    state.client_id = NULL;
    state.conn_state = IDLE;
    state.sock_fd = -1;
    state.server_addr = host_name;
}

int hello(cmd_line* _cmd_line){
    if(state.conn_state != IDLE){
        send(state.sock_fd, "nok state\n", 10, 0);
        close(state.sock_fd);
        init_state();
        return -1;
    }
    char client_id_num_to_str [10];
    sprintf(client_id_num_to_str, "%d", client_id_num);
    char to_send [2000];
    memset(to_send, 0, 2000);
    strcat(to_send, "hello ");
    strcat(to_send, client_id_num_to_str);
    strcat(to_send, "\n");
    if(send(state.sock_fd, to_send, strlen(to_send), 0) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    state.conn_state = CONNECTED; 
    state.client_id = client_id_num_to_str;
    client_id_num++;
    printf("Client %s is connected\n",state.client_id);
    return 0;
}

int bye(cmd_line* _cmd_line){
    if(state.conn_state != CONNECTED){
        send(state.sock_fd, "nok state\n", 10, 0);
        close(state.sock_fd);
        init_state();
        return -1;
    }
    if(send(state.sock_fd, "bye\n", 4, 0) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    close(state.sock_fd);
    printf("Client %d is disconnected\n", (client_id_num-1));
    init_state();
    return 0;
}

int ls (cmd_line* _cmd_line){
    if(state.conn_state != CONNECTED){
        send(state.sock_fd, "nok state\n", 10, 0);
        close(state.sock_fd);
        init_state();
        return -1;
    }
    char* files_lst = list_dir();
    if(files_lst == NULL){
        send(state.sock_fd, "nok filesystem\n", 10, 0);
        return -1;
    }
    if(send(state.sock_fd, "ok\n", 3, 0) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    if(send(state.sock_fd, files_lst, strlen(files_lst), 0) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    char curr_dir [LS_RESP_SIZE];
    printf("listed file at %s\n", getcwd(curr_dir,LS_RESP_SIZE));
    return 0 ;
    
}

int exec(cmd_line* _cmd_line){
    if(strcmp(_cmd_line -> arguments[0],"hello") == 0) 
        return hello(_cmd_line);
    if(strcmp(_cmd_line -> arguments[0],"ls") == 0) 
        return ls(_cmd_line);
    if(strcmp(_cmd_line -> arguments[0],"bye") == 0) 
        return bye(_cmd_line);
    else{
        fprintf(stderr, "%s | Error: Unknown message %s\n" ,state.client_id, _cmd_line -> arguments[0]);
        send(state.sock_fd, "nok message\n", strlen("nok message\n"), 0);
        close(state.sock_fd);
        init_state();
    }
        
    return 0;
}

int main(int argc, char* argv[]){
    struct sockaddr_storage soc_addr;
    socklen_t addr_len = sizeof(soc_addr);
    
    if(argc > 1){
        if(strstr(argv[1], "-d") != NULL) debug = 1;
    }
    int optval = 1;
    init_state();
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* res = NULL;
    getaddrinfo(NULL, "2018", &hints, &res); 
    int sock = socket(res -> ai_family, res -> ai_socktype, res -> ai_protocol);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //BIND
    bind(sock, res -> ai_addr, res -> ai_addrlen);
    freeaddrinfo(res);

    while (TRUE){
        char client_msg [100];
        memset(client_msg, 0, 100);
        if(state.conn_state != CONNECTED){
            listen (sock, 1); 
            state.sock_fd = accept(sock, (struct sockaddr*)&soc_addr, &addr_len);
        }
        recv(state.sock_fd , client_msg , 100, 0);
        print_debug_msg(client_msg);
        
        if(strcmp(client_msg,"\n")==0)
                continue;
        
        if(client_msg[0] == '\0'){
            close(state.sock_fd);
            printf("Client %d is disconnected\n", (client_id_num-1));
            init_state();
            continue; //handle control C
        }
            
        cmd_line* _cmd_line = parse_cmd_lines(client_msg);
        
        exec(_cmd_line);
        
        free_cmd_lines(_cmd_line);
        
    }
    
    return 0;
}
