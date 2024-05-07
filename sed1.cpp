#include "Common.h"

#define SERVER_IP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512
#define TOTAL_PACKETS 6 // 전체 패킷 개수

typedef struct {
    int seq_num;
    char message[BUFSIZE];
} Packet;

int main(int argc, char *argv[]) {
    int retval;

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 서버 정보 설정
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serveraddr.sin_port = htons(SERVERPORT);

    // 서버에 연결
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    Packet packets[TOTAL_PACKETS];
    int next_seq_num = 0;
    int acked[TOTAL_PACKETS] = {0}; // 각 패킷의 ACK 상태를 추적

    // 처음 4개 패킷 생성
    for (int i = 0; i < 4 && i < TOTAL_PACKETS; i++) {
        packets[i].seq_num = i;
        sprintf(packets[i].message, "Data for packet %d", i);
    }

    // 처음 4개 패킷 전송
    for (int i = 0; i < 4 && i < TOTAL_PACKETS; i++) {
        retval = send(sock, (char *)&packets[i], sizeof(Packet), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
        printf("* \"packet %d\" is transmitted.\n", packets[i].seq_num);
    }

    // 이후 패킷 전송
    while (next_seq_num < TOTAL_PACKETS) {
        // ACK 기다리기
        int ack;
        retval = recv(sock, (char *)&ack, sizeof(int), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0) {
            printf("Server disconnected.\n");
            break;
        }

        // 올바른 ACK인지 확인
        if (ack >= 0 && ack < TOTAL_PACKETS) {
            printf("* \"ACK %d\" is received.\n", ack);
            acked[ack] = 1; // 해당 ACK 처리 완료
            // 다음 패킷 전송
            while (next_seq_num < TOTAL_PACKETS && acked[next_seq_num]) {
                next_seq_num++;
            }
            if (next_seq_num < TOTAL_PACKETS) {
                retval = send(sock, (char *)&packets[next_seq_num], sizeof(Packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                printf("* \"packet %d\" is transmitted.\n", packets[next_seq_num].seq_num);
            }
        } else {
            printf("* Unexpected ACK received: %d.\n", ack);
        }
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
