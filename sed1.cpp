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

    Packet packet;
    int next_seq_num = 0;
    int expected_ack_num = 0;

    while (1) {
        // 패킷 생성
        packet.seq_num = next_seq_num;
        sprintf(packet.message, "Data for packet %d", next_seq_num);

        // 패킷 전송
        retval = send(sock, (char *)&packet, sizeof(Packet), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
        printf("* \"packet %d\" is transmitted.\n", packet.seq_num);

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
        if (ack == expected_ack_num) {
            printf("* \"ACK %d\" is received.\n", ack);
            next_seq_num = (next_seq_num + 1) % TOTAL_PACKETS;
            expected_ack_num = (expected_ack_num + 1) % TOTAL_PACKETS;
            // 모든 패킷을 전송한 경우 루프 탈출
            if (next_seq_num == 0)
                break;
        } else {
            printf("* Unexpected ACK received. Resending packet %d.\n", packet.seq_num);
        }
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
