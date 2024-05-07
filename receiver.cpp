#include "../Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512
#define WINDOW_SIZE 4
#define TIMEOUT_INTERVAL 5

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

        // Go-Back-N 알고리즘을 위한 변수
        int expected_seq_num = 0;
        int base = 0;
        int timeout_count = 0;

		// 클라이언트와 데이터 통신
		while (1) {
            // 타임아웃 체크
            if (timeout_count >= TIMEOUT_INTERVAL) {
                printf("Timeout occurred. Resending ACK for \"%d\"\n", base);
                char ack_msg[20];
                sprintf(ack_msg, "ACK %d", base - 1);
                retval = send(client_sock, ack_msg, strlen(ack_msg), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                timeout_count = 0;
            }

			// 데이터 받기
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			// 받은 데이터 출력
			buf[retval] = '\0';
            int seq_num;
            sscanf(buf, "packet %d", &seq_num);
            if (seq_num == expected_seq_num) {
                // 패킷 정상 수신
                printf("* \"packet %d\" is received. ", seq_num);
                printf("\"ACK %d\" is transmitted.\n", seq_num);
                char ack_msg[20];
                sprintf(ack_msg, "ACK %d", seq_num);
                retval = send(client_sock, ack_msg, strlen(ack_msg), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
                expected_seq_num++;
                timeout_count = 0; // 정상적인 패킷을 받으면 타임아웃 초기화
            } else {
                // 패킷 손실
                printf("* \"packet %d\" is received and dropped. ", seq_num - 1);
                printf("\"ACK %d\" is retransmitted.\n", base - 1);
                char ack_msg[20];
                sprintf(ack_msg, "ACK %d", base - 1);
                retval = send(client_sock, ack_msg, strlen(ack_msg), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
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
