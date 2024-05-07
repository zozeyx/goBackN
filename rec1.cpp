#include "../Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define PACKET_COUNT 6

void receive_packet(int seq_num) {
    // 패킷을 수신하는 함수
    printf("\"packet %d\" is received.\n", seq_num);
}

void send_ack(int ack_num, SOCKET client_sock, struct sockaddr_in clientaddr) {
    // ACK를 송신하는 함수
    char ack_msg[BUFSIZE];
    sprintf(ack_msg, "ACK %d", ack_num);
    send(client_sock, ack_msg, strlen(ack_msg), 0);
    printf("\"ACK %d\" is transmitted.\n", ack_num);
}

void timeout_event(int seq_num) {
    // Timeout 이벤트를 처리하는 함수
    printf("\"packet %d\" is timeout.\n", seq_num);
    send_packet(seq_num);
}

int main(int argc, char *argv[])
{
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
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // 접속한 클라이언트 정보 출력
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
               addr, ntohs(clientaddr.sin_port));

        // 클라이언트와 데이터 통신
        int expected_seq = 0;
        while (1) {
            // 데이터 받기
            retval = recv(client_sock, buf, BUFSIZE, 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            } else if (retval == 0)
                break;

            // 받은 데이터 출력
            buf[retval] = '\0';
            receive_packet(expected_seq);
            expected_seq = (expected_seq + 1) % PACKET_COUNT;

            // ACK 보내기
            send_ack(expected_seq - 1, client_sock, clientaddr);

            // Timeout 이벤트 처리
            static int timer = 0;
            timer++;
            if (timer == TIMEOUT_INTERVAL) {
                timeout_event(expected_seq - 1);
                timer = 0;
            }
        }

        // 소켓 닫기
        close(client_sock);
        printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
               addr, ntohs(clientaddr.sin_port));
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
