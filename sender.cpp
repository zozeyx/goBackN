#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE 512
#define WINDOW_SIZE 4
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

    // 패킷 수신 및 ACK 전송
    int expected_seq_num = 0;
    int ignore_packet_2 = 0; // 패킷 2를 무시하는 플래그
    while (expected_seq_num < TOTAL_PACKETS) {
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

            printf("* \"packet %d\" is received.\n", packet.seq_num);

            // 패킷 2를 무시하고 패킷 3부터 드랍시키고 ACK 1 전송
            if (packet.seq_num == 2 && !ignore_packet_2) {
                ignore_packet_2 = 1;
                continue;
            } else if (packet.seq_num >= 3) {
                printf("* \"packet %d\" is received and dropped.\n", packet.seq_num);
                retval = send(client_sock, (char *)&expected_seq_num, sizeof(int), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                continue;
            }

            // 기대한 시퀀스 번호와 다르면 재전송
            if (packet.seq_num != expected_seq_num) {
                printf("* \"packet %d\" is received and dropped.\n", packet.seq_num);
            } else {
                // 기대한 시퀀스 번호와 같으면 ACK 전송
                printf("* \"packet %d\" is received and delivered.\n", packet.seq_num);
                expected_seq_num++;
            }

            // ACK 전송
            printf("* \"ACK %d\" is transmitted.\n", expected_seq_num - 1);
            retval = send(client_sock, (char *)&expected_seq_num, sizeof(int), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }

            // 모든 패킷을 수신했을 경우 루프 탈출
            if (expected_seq_num == TOTAL_PACKETS)
                break;
        }

        // 소켓 닫기
        close(client_sock);
    }

    // 소켓 닫기
    close(listen_sock);
    return 0;
}
