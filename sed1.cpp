#include "../Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define PACKET_COUNT 6

// 패킷을 전송하는 함수
void send_packet(int seq_num) {
    // 패킷을 전송하는 코드 추가
    char packet_msg[BUFSIZE];
    sprintf(packet_msg, "packet %d", seq_num);
    send(sock, packet_msg, strlen(packet_msg), 0);
    printf("\"packet %d\" is transmitted.\n", seq_num);
}

// Timeout 이벤트를 처리하는 함수
void timeout_event(int seq_num) {
    // Timeout 이벤트를 처리하는 코드 추가
    printf("\"packet %d\" is timeout.\n", seq_num);
    send_packet(seq_num);
}

int main(int argc, char *argv[])
{
    int retval;

    // 명령행 인수가 있으면 IP 주소로 사용
    if (argc > 1) SERVERIP = argv[1];

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
    char buf[BUFSIZE + 1];
    int len;

    // 서버와 데이터 통신
    int packets_sent = 0;
    while (packets_sent < PACKET_COUNT) {
        // 패킷 전송
        send_packet(packets_sent);
        packets_sent++;
        usleep(1); // 1 time unit 대기

        // Timeout 이벤트 처리
        static int timer = 0;
        timer++;
        if (timer == TIMEOUT_INTERVAL) {
            timeout_event(packets_sent - 1);
            timer = 0;
        }
    }

    // 소켓 닫기
    close(sock);
    return 0;
}
