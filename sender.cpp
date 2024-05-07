#include "Common.h"

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512
#define WINDOW_SIZE 4
#define TOTAL_PACKETS 6 // 전체 패킷 개수
#define TIMEOUT_PACKET 2 // 타임아웃이 발생할 패킷 번호

typedef struct {
    int seq_num;
    char message[BUFSIZE];
} Packet;

int main(int argc, char *argv[]) {
    int retval;

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 데이터 통신에 사용할 변수
    Packet packet;
    int next_seq_num = 0;

    // 서버와 데이터 통신
    while (next_seq_num < TOTAL_PACKETS) {
        // 패킷 전송
        for (int i = next_seq_num; i < next_seq_num + WINDOW_SIZE && i < TOTAL_PACKETS; i++) {
            packet.seq_num = i;
            sprintf(packet.message, "packet %d", i);
            retval = send(sock, (char *)&packet, sizeof(Packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
            printf("* \"packet %d\" is transmitted.\n", packet.seq_num);
        }

        // ACK 확인
        int ack_num;
        retval = recv(sock, (char *)&ack_num, sizeof(int), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0)
            break;

        // 중복 ACK 무시
        if (ack_num == next_seq_num - 1) {
            printf("* \"ACK %d\" is received and ignored.\n", ack_num);
            continue;
        }

        // 패킷의 ACK 상태 업데이트
        next_seq_num = ack_num + 1;

        // 타임아웃 패킷 이후 재전송
        if (next_seq_num == TIMEOUT_PACKET) {
            printf("* packet %d is timeout.\n", TIMEOUT_PACKET);
        }
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
