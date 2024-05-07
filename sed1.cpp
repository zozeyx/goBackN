#include "Common.h"

#define SERVER_IP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512
#define WINDOW_SIZE 3 // 윈도우 사이즈
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
    int base = 0;
    int acked[TOTAL_PACKETS] = {0}; // 각 패킷의 ACK 상태를 추적

    // 패킷 생성
    for (int i = 0; i < TOTAL_PACKETS; i++) {
        packets[i].seq_num = i;
        sprintf(packets[i].message, "Data for packet %d", i);
    }

    while (base < TOTAL_PACKETS) {
        // 윈도우 내 패킷 전송
        for (int i = base; i < base + WINDOW_SIZE && i < TOTAL_PACKETS; i++) {
            if (!acked[i]) {
                retval = send(sock, (char *)&packets[i], sizeof(Packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                printf("* \"packet %d\" is transmitted.\n", packets[i].seq_num);
            }
        }

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
        if (ack >= base && ack < base + WINDOW_SIZE) {
            printf("* \"ACK %d\" is received.\n", ack);
            acked[ack] = 1; // 해당 ACK 처리 완료
            // 윈도우 범위 내의 모든 패킷이 ACK를 받았는지 확인
            while (acked[base])
                base++;
        } else {
            printf("* Unexpected ACK received: %d.\n", ack);
        }
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
