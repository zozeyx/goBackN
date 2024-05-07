#include "Common.h"

#define SERVERPORT 9000 
#define BUFSIZE    512  
#define PACKET_SIZE 8

int seq_num = -1; 
int packetCompair = -1;
char packet_str[9][20];
int packet[9] = {0,1,2,3,4,5,6,7,8};

int main(int argc, char *argv[]) {
    // 패킷 문자열 초기화
    for (int i = 0; i < 9; i++) {
        sprintf(packet_str[i], "packet %d", packet[i]);
    }

    int retval;

    // 소켓 생성
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);   
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");  

    // bind()
    struct sockaddr_in serveraddr;     
    memset(&serveraddr, 0, sizeof(serveraddr));     
    serveraddr.sin_family = AF_INET;    
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);     
    serveraddr.sin_port = htons(SERVERPORT);    
    
    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));  
    if (retval == SOCKET_ERROR) err_quit("bind()");  

    // listen()
    retval = listen(listen_sock, SOMAXCONN);   
    if (retval == SOCKET_ERROR) err_quit("listen()");  

    // 데이터 통신에 사용할 변수
    SOCKET client_sock;   
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    while (1) {
        // 클라이언트 연결 수락
        addrlen = sizeof(clientaddr);   
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen); 
        if (client_sock == INVALID_SOCKET) { 
            err_display("accept()");
            break;
        }

        // 데이터 통신
        for (int i = 0; i < 4; i++) {
            char received_data[BUFSIZE + 1];
            int received_len = 0;
            while (received_len < PACKET_SIZE) {
                int ret = recv(client_sock, received_data + received_len, PACKET_SIZE - received_len, 0);
                if (ret == SOCKET_ERROR) {
                    err_display("recv()");
                    break;
                } else if (ret == 0) {
                    break;
                } else {
                    received_len += ret;
                }
            }

            // 수신한 데이터 처리
            if (strncmp(received_data, packet_str[2], PACKET_SIZE) == 0) {
                seq_num--;
                continue;
            }

            seq_num++;

            if (strncmp(received_data, packet_str[seq_num], PACKET_SIZE) != 0) {
                printf("\"%s\" is received and dropped.", received_data); 

                seq_num--;

                char ack_bufTemp[BUFSIZE];
                sprintf(ack_bufTemp, "ACK %d", seq_num);
                retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
                if (retval == SOCKET_ERROR) {   
                    err_display("send()");
                    break;
                }

                printf("\"ACK %d\" is transmitted.\n", seq_num); 
            } else {
                printf("\"%s\" is received.", received_data);
                char ack_buf[BUFSIZE];
                sprintf(ack_buf, "ACK %d", seq_num); 
                retval = send(client_sock, ack_buf, strlen(ack_buf), 0);  
                if (retval == SOCKET_ERROR) {   
                    err_display("send()");
                    break;
                }
                printf("\"ACK %d\" is transmitted.\n", seq_num); 
            }
        }
        
        // 소켓 닫기
        close(client_sock);
        break; // 하나의 클라이언트와 통신 후에 루프 종료
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
