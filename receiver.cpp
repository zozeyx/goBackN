#include "Common.h"

#define SERVERPORT 9000 
#define BUFSIZE    512  
#define PACKET_SIZE 8

int packetNumber = -1; 
int packetCompair = -1;
char packet_str[9][20];
int packet[9] = {0,1,2,3,4,5,6,7,8};

ssize_t recv_fixlen(int sockfd, char *buf, size_t len, int flags) {
	int received_len = 0;
	int ret;
	while(received_len < len) {
		ret = recv(sockfd, &buf[received_len], len - received_len, flags);
		if (ret == SOCKET_ERROR) {
			return ret;
		} else if (ret == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
			return ret;
		} else {
			received_len += ret;
		}
	}
}

int main(int argc, char *argv[])
{
	for (int i = 0; i < 9; i++) {	//packet_str에 packet 0 이런식으로 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }

	int retval;

	
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
		
		addrlen = sizeof(clientaddr);   
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen); 
        //클라이언트 연결을 수락하고, 클라이언트와의 통신을 위한 새로운 소켓을 생성
		if (client_sock == INVALID_SOCKET) {    //소켓이 유효하지 않은 경우에 오류 처리
			err_display("accept()");
			break;
		}

		// 클라이언트와 데이터 통신
		//while (1) {
		for(int i=0; i<4; i++){
			// 데이터 받기
			char received_data[BUFSIZE + 1]; // 1을 더해서 NULL 문자를 추가할 공간을 확보

        	retval = recv_fixlen(client_sock, received_data, PACKET_SIZE, 0); // 클라이언트로부터 데이터를 수신
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  // 수신 중 오류가 발생한 경우 오류 메시지 출력
        		break;
    		} else if (retval == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
        		break;        		
			}
			packetNumber++;		//데이터를 제대로 받으면 packetNumber을  ++해준다.	packet 0 success, packet 1 success 근데 여기서 packet2도 받았기 때문에 일단 2다.
			//마지막 packet3을 받고 다시 패킷 넘버는 2가 된다.

			if(strncmp(received_data, packet_str[2], PACKET_SIZE) == 0){		//packet2 유실되었다는 가정
				packetNumber--;		//패킷2는 유실되었다는 가정이기 때문에 packetNumber를 하나 뺀다. 	현재 패킷 넘버는 1
				continue;		//그리고 continue를 사용하여 밑은 다 패스하고 다음 데이터를 받는다.
			}

			if (strncmp(received_data, packet_str[packetNumber], PACKET_SIZE) != 0) {		//패킷이 유실되어 중간에 하나가 도착하지 않았고 받은 패킷이랑 packetNumber랑 일치하지 않는다. 받은건 3 packetNumber는 2
				printf("\"%s\" is received and dropped.", received_data); 	

				packetNumber = packetNumber - 1;	//못받은 패킷의 이전 패킷 번호를 구하기 위해 현재 패킷넘버 2에서 1을 뺀 값을 저장한다.
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", packetNumber);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", packetNumber); 

    		}else{		//패킷이 순서대로 제대로 온 경우
				printf("\"%s\" is received.", received_data);
				char ack_buf[BUFSIZE];

				sprintf(ack_buf, "ACK %d", packetNumber); // "ACK"와 packetNumber를 문자열로 결합하여 ack_buf에 저장

    	        // 데이터 보내기
     	       	retval = send(client_sock, ack_buf, strlen(ack_buf), 0);  
      	    	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
            	printf("\"ACK %d\" is transmitted.\n", packetNumber); 

			}
		}		//여기까지 packetNumber는 1
		
		for(int i=0; i<2; i++){
			char received_data[BUFSIZE + 1]; // 1을 더해서 NULL 문자를 추가할 공간을 확보

        	retval = recv_fixlen(client_sock, received_data, PACKET_SIZE, 0); // 클라이언트로부터 데이터를 수신
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  // 수신 중 오류가 발생한 경우 오류 메시지 출력
        		break;
    		} else if (retval == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
        		break;        		
			}
			packetNumber++;	

			if (strncmp(received_data, packet_str[packetNumber], PACKET_SIZE) != 0) {		//패킷이 유실되어 중간에 하나가 도착하지 않았고 받은 패킷이랑 packetNumber랑 일치하지 않는다. 받은건 3 packetNumber는 2
				printf("\"%s\" is received and dropped.", received_data); 	

				packetNumber = packetNumber - 1;	//못받은 패킷의 이전 패킷 번호를 구하기 위해 현재 패킷넘버 2에서 1을 뺀 값을 저장한다.
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", packetNumber);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", packetNumber); 
			}

		}
		//printf("%d", packetNumber);
		
		for(int i=0; i<4; i++){
			char received_data[BUFSIZE + 1]; // 1을 더해서 NULL 문자를 추가할 공간을 확보

        	retval = recv_fixlen(client_sock, received_data, PACKET_SIZE, 0); // 클라이언트로부터 데이터를 수신
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  // 수신 중 오류가 발생한 경우 오류 메시지 출력
        		break;
    		} else if (retval == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
        		break;        		
			}
			packetNumber++;	
			//printf("trace : %s %d %s\n", received_data, packetNumber, packet_str[packetNumber]);
			if (strncmp(received_data, packet_str[packetNumber], PACKET_SIZE) != 0) {		//패킷이 유실되어 중간에 하나가 도착하지 않았고 받은 패킷이랑 packetNumber랑 일치하지 않는다. 받은건 3 packetNumber는 2
				printf("\"%s\" is received and dropped.", received_data); 	

				packetNumber = packetNumber - 1;	//못받은 패킷의 이전 패킷 번호를 구하기 위해 현재 패킷넘버 2에서 1을 뺀 값을 저장한다.
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", packetNumber);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", packetNumber); 

    		}else{		//패킷이 순서대로 제대로 온 경우
				printf("\"%s\" is received and delivered.", received_data);
				char ack_buf[BUFSIZE];

				sprintf(ack_buf, "ACK %d", packetNumber); // "ACK"와 packetNumber를 문자열로 결합하여 ack_buf에 저장

    	        // 데이터 보내기
     	       	retval = send(client_sock, ack_buf, strlen(ack_buf), 0);  
      	    	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
            	printf("\"ACK %d\" is transmitted.\n", packetNumber); 

			}
		}
		
		

		// 소켓 닫기
		close(client_sock);
		break;
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
