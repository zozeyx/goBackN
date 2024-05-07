#include "Common.h"

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512
#define WINDOW_SIZE 4
#define TIMEOUT_INTERVAL 5
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
    int base = 0;
    int timer = 0;
    int timeout_packet = -1; // timeout이 발생한 패킷
    int last_ack_num = -1; // 마지막으로 받은 ACK 번호

    // 서버와 데이터 통신
    while (base < TOTAL_PACKETS) {
        // 패킷 전송
        if (base == 0 || base == 2) {
            for (int i = base; i < base + WINDOW_SIZE && i < TOTAL_PACKETS; ++i) {
                packet.seq_num = i;
                sprintf(packet.message, "packet %d", i);
                retval = send(sock, (char *)&packet, sizeof(Packet), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                printf("* \"packet %d\" is transmitted.\n", packet.seq_num);
                next_seq_num++;
            }
        }

        // ACK 확인
        int ack_num;
        retval = recv(sock, (char *)&ack_num, sizeof(int), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        } else if (retval == 0)
            break;

        // 동일한 ACK 무시
        if (ack_num == last_ack_num) {
            printf("* \"ACK %d\" is received and ignored.\n", ack_num);
            continue;
        }

        printf("* \"ACK %d\" is received.\n", ack_num);
        last_ack_num = ack_num;

        // 패킷의 ACK 상태 업데이트
        if (ack_num >= base) {
            base = ack_num + 1;
        }

        // 타임아웃 처리
        if (timer >= TIMEOUT_INTERVAL || timeout_packet != -1) {
            if (timeout_packet == -1) {
                printf("* packet %d is timeout.\n", base);
                timeout_packet = base;
            }
            next_seq_num = timeout_packet; // 재전송을 위해 timeout이 발생한 패킷부터 재전송
            timer = 0;
            timeout_packet = -1; // timeout 패킷 재설정
        }

        // 타이머 초기화
        timer++;
    }

    // 소켓 닫기
    closesocket(sock);
    WSACleanup();
    return 0;
}
