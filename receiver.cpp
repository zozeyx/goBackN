#include "Common.h"
#include <stdlib.h> // for rand() and srand()
#include <time.h>   // for time()

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

    // 패킷 2를 받았는지 여부를 나타내는 변수
    int received_packet_2 = 0;

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

            // 패킷이 2번일 때 드롭하고, 이후 패킷들은 재전송 요청을 보내도록 함
            if (packet.seq_num == 2 && !received_packet_2) {
                received_packet_2 = 1;
                continue; // 패킷 2를 받음
            } else {
                // 재전송 요청을 보냄
                printf("* \"packet %d\" is received and dropped. \"ACK %d\" is retransmitted.\n", packet.seq_num, (packet.seq_num - 1 + TOTAL_PACKETS) % TOTAL_PACKETS);
                retval = send(client_sock, (char *)&((packet.seq_num - 1 + TOTAL_PACKETS) % TOTAL_PACKETS), sizeof(int), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
            }

            // 모든 패킷을 수신했을 경우 루프 탈출
            if (packet.seq_num == TOTAL_PACKETS - 1)
                break;
        }

        // 소켓 닫기
        close(client_sock);

        // 패킷 2를 다시 받을 수 있도록 초기화
        received_packet_2 = 0;
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
