#include "Common.h"

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
    Packet packet;

    int expected_seq_num = 0; // 기대하는 시퀀스 번호

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // 클라이언트와 데이터 통신
        while (1) {
            // 데이터 받기
            retval = recv(client_sock, (char *)&packet, sizeof(Packet), 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            } else if (retval == 0)
                break;

            // 패킷 시퀀스 번호 확인
            printf("* \"packet %d\" is received.\n", packet.seq_num);

            // 드랍된 패킷인 경우
            if (packet.seq_num == 2) {
                printf("* \"packet %d\" is dropped.\n", packet.seq_num);
                continue; // 패킷 드랍 후 다음 패킷 수신 대기
            }

            // 패킷 시퀀스 번호가 기대하는 것과 일치하는지 확인
            if (packet.seq_num == expected_seq_num) {
                // ACK 보내기
                retval = send(client_sock, (char *)&packet.seq_num, sizeof(int), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                printf("* \"ACK %d\" is transmitted.\n", packet.seq_num);

                // 다음 기대하는 시퀀스 번호로 업데이트
                expected_seq_num = (expected_seq_num + 1) % TOTAL_PACKETS;

                // 모든 패킷을 수신했을 경우 루프 탈출
                if (packet.seq_num == TOTAL_PACKETS - 1)
                    break;
            } else {
                // 기대하는 시퀀스 번호와 일치하지 않는 패킷은 무시하고 ACK 재전송 요청
                printf("* Unexpected packet received. Waiting for packet %d.\n", expected_seq_num);
                retval = send(client_sock, (char *)&expected_seq_num, sizeof(int), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                printf("* ACK %d is retransmitted.\n", expected_seq_num);
            }
        }

        // 소켓 닫기
        close(client_sock);
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
