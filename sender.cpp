#include "../Common.h"

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define WINDOW_SIZE 4
#define TIMEOUT_INTERVAL 5

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

    // Go-Back-N 알고리즘을 위한 변수
    int base = 0;
    int next_seq_num = 0;
    int window[WINDOW_SIZE] = {0};
    bool ack_received = false;

	// 서버와 데이터 통신
	while (1) {
        // 송신
        while (next_seq_num < base + WINDOW_SIZE) {
            // 데이터 보내기
            printf("* \"packet %d\" is transmitted.\n", next_seq_num);
            sprintf(buf, "packet %d", next_seq_num);
            retval = send(sock, buf, (int)strlen(buf), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }

            // 윈도우에 송신한 패킷 번호 추가
            window[next_seq_num % WINDOW_SIZE] = next_seq_num;
            next_seq_num++;
        }

        // 수신
        while (1) {
            // 데이터 받기
            retval = recv(sock, buf, BUFSIZE, 0);
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0)
                break;

            // 받은 데이터 출력
            buf[retval] = '\0';
            printf("* \"%s\" is received. ", buf);
            printf("* \"ACK %d\" is transmitted.\n", base);
            retval = send(sock, buf, (int)strlen(buf), 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }

            // ACK 처리
            int ack_num;
            sscanf(buf, "ACK %d", &ack_num);
            if (ack_num >= base && ack_num < next_seq_num) {
                window[ack_num % WINDOW_SIZE] = -1; // ACK를 받은 패킷은 윈도우에서 제거
                if (ack_num == base) {
                    base++;
                }
            }

            // 모든 패킷이 ACK를 받았는지 확인
            ack_received = true;
            for (int i = base; i < next_seq_num; i++) {
                if (window[i % WINDOW_SIZE] != -1) {
                    ack_received = false;
                    break;
                }
            }
            if (ack_received) break;
        }
	}

	// 소켓 닫기
	close(sock);
	return 0;
}
