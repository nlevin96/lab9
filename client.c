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
#include "common.h"
#include "line_parser.h"
#include "limits.h"
#define TRUE 1
#define FALSE 0
#define LS_RESP_SIZE 2048

client_state state;
int debug = 0;

void print_debug_msg(char* msg){
    if(debug){
        fprintf(stderr, "%s | Log: %s", state.server_addr, msg);
    }
}

void init_state(){
    state.client_id = NULL;
    state.conn_state = IDLE;
    state.sock_fd = -1;
    state.server_addr = "nil";
}

int conn(cmd_line* _cmd_line){
    struct sockaddr_in soc_addr;
    soc_addr.sin_family = AF_INET; 
    soc_addr.sin_port = htons(2018);
    char* client_msg = "hello\n";
    char server_msg[2000];
    memset(server_msg, 0, 2000);
    
    int i = 6;
    int tmp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(state.conn_state != IDLE){
        perror("ERROR! conn_state != IDLE\n");
        return -2;
    }
    if(_cmd_line -> arguments[1] == NULL) {
        perror("ERROR! no server address\n");
        return -1;
    }
    if(connect(tmp_fd, (struct sockaddr *)&soc_addr, sizeof(soc_addr)) == -1){
        perror("ERROR! connection failed\n");
        return -1;
    }
    if(send(tmp_fd, client_msg, strlen(client_msg), 0) == -1){ 
        perror("ERROR! send failed\n");
        return -1;
    }
    state.conn_state = CONNECTING;
    if(recv(tmp_fd, server_msg, 2000, 0) == -1){ 
        perror("ERROR! recv failed\n");
        return -1;
    }
    if(strstr(server_msg, "nok ") != NULL){
        close(tmp_fd);
        init_state();
        fprintf(stderr, "Server Error: %s", server_msg + 4);
        return -1;
    }
    if(strstr(server_msg, "hello ") == NULL){
        perror("ERROR! got wrong response\n");
        return -1;
    }
    //get server id
    char s_id [strlen(_cmd_line -> arguments[1])];
    memcpy(s_id, _cmd_line -> arguments[1], strlen(_cmd_line -> arguments[1])+1);
    //get client id
    char c_id [strlen(server_msg)-6];
    while(server_msg[i] != '\n'){
        c_id[i-6] = server_msg[i];
        i++;
    }
    c_id[i-6] = '\0';
    state.client_id = c_id;
    state.conn_state = CONNECTED;
    state.sock_fd = tmp_fd;
    state.server_addr = s_id;
    print_debug_msg(server_msg);
    return 0;
}

int bye(){
    char* client_msg_bye = "bye\n";
    if(state.conn_state != CONNECTED){
        perror("ERROR! conn_state != CONNECTED\n");
        return -2;
    }
    int num_of_bytes;
    if( (num_of_bytes = send(state.sock_fd, client_msg_bye, strlen(client_msg_bye), 0) ) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    //printf("num_of_bytes is %d> \n",num_of_bytes);
    close(state.sock_fd);
    init_state();
    return 0;
}

int ls(cmd_line* _cmd_line){
    char* client_msg_ls = "ls\n";
    char nok_ok [3];
    char server_msg[LS_RESP_SIZE];
    memset(nok_ok, 0, 3);
    memset(server_msg, 0, LS_RESP_SIZE);
    if(state.conn_state != CONNECTED){
        perror("ERROR! conn_state != CONNECTED\n");
        return -2;
    }
    int num_of_bytes;
    if( (num_of_bytes = send(state.sock_fd, client_msg_ls, strlen(client_msg_ls), 0) ) == -1){
        perror("ERROR! send failed\n");
        return -1;
    }
    if(recv(state.sock_fd, nok_ok, 3, 0) == -1){ 
        perror("ERROR! recv failed\n");
        return -1;
    }
    print_debug_msg(server_msg);
    if(recv(state.sock_fd, server_msg, LS_RESP_SIZE, 0) == -1){ 
        perror("ERROR! recv failed\n");
        return -1;
    }
    if(strstr(nok_ok, "nok") != NULL){
        close(state.sock_fd);
        init_state();
        fprintf(stderr, "Server Estrcmprror: %s", server_msg);
        return -1;
    }
    if(strstr(nok_ok,"ok") == NULL){
        perror("ERROR! got wrong response\n");
        return -1;
    }
    printf("%s",server_msg);
    return 0;
    
}

int exec(cmd_line* _cmd_line){
    if(strcmp(_cmd_line -> arguments[0],"conn") == 0) 
        return conn(_cmd_line);
    if(strcmp(_cmd_line -> arguments[0],"bye") == 0) 
        return bye();
    if(strcmp(_cmd_line -> arguments[0], "ls") == 0)
        return ls(_cmd_line);
    return 0;
}

int main(int argc, char* argv[]){
    char* line;
    state.server_addr = "nil";
    state.conn_state = IDLE;
    state.client_id = NULL;
    state.sock_fd = -1;
    
    if(argc > 1){
        if(strstr(argv[1], "-d") != NULL) debug = 1;
    }
    
    while (TRUE){
        printf("server:%s> ",state.server_addr);
        char in_buff [2048];
        line = fgets(in_buff,2048,stdin);
        
        if(strcmp(line,"\n")==0)
            continue;
        
        cmd_line* _cmd_line = parse_cmd_lines(line);
        
        if(strcmp(_cmd_line -> arguments[0],"quit") == 0){
            if(state.conn_state == CONNECTED) bye();
            free_cmd_lines(_cmd_line);
            return 0;
        }
        exec(_cmd_line);
    
        free_cmd_lines(_cmd_line);
        
    }
    
    
    
    return 0;
}
