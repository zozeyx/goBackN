#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1"; 

#define SERVERPORT 9000     
#define BUFSIZE    512      
#define ACK_SIZE	5
#define WINDOW_SIZE 4

int packet[9] = {0,1,2,3,4,5,6,7,8};	//패킷선언
char packet_str[9][20];
int ackCount = -1; // ack 받은 횟수 체크
char ack[100];
int count[9] = {0}; //timeout 카운트
int ackCheck[9] = {0}; // 패킷에 대한 ack 받았는지
int escape = 0;	

int recvlen(int sockfd, char *buf, size_t len, int flags) { //데이터 길이 반환
	int data_len = 0;
	int ret;
	while(data_len < len) {
		ret = recv(sockfd, &buf[data_len], len - data_len, flags);
		if (ret == SOCKET_ERROR) {
			return ret;
		} else if (ret == 0) { 
			return ret;
		} else {
			data_len += ret;
		}
	}
	return data_len;
}

int main(int argc, char *argv[]){

	int retval;
	int start = 0;
	int end = -1;

	for (int i = 0; i < 9; i++) {	//packet0, packet1, packet2 ... 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }


	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); 
	if (sock == INVALID_SOCKET) err_quit("socket()");

	struct sockaddr_in serveraddr;  
	memset(&serveraddr, 0, sizeof(serveraddr));     
	serveraddr.sin_family = AF_INET;    
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);   
	serveraddr.sin_port = htons(SERVERPORT);    

	retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));    
	if (retval == SOCKET_ERROR) err_quit("connect()");  


	for(int i=0; i<WINDOW_SIZE; i++){		
		end++;
		retval = send(sock, packet_str[end], strlen(packet_str[end]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		for(int j=0; j<=end; j++){
			if(ackCheck[j]==1){		
				continue;
			}
			count[j]++;
			if(count[j]>4){
				printf("\"%s\" is timeout.\n", packet_str[j]);
				break;
			}
		}
	
		printf("\"%s\" is transmitted.\n", packet_str[end]);		
	}	

	for(int i=0; i<5; i++){
		// 데이터 받기
		char buf[BUFSIZE + 1];

		retval = recvlen(sock, buf, ACK_SIZE, 0);  
		if (retval == SOCKET_ERROR) {   
			err_display("recv()");
			break;
		} else if (retval == 0) {   
			break;
		} else if (strncmp(buf, ack, ACK_SIZE) != 0) {	
			strcpy(ack, buf);	
			printf("\"%s\" is received.", ack);	
			ackCount++;
			ackCheck[ackCount] = 1;
			start++;	
			end++;	

			retval = send(sock, packet_str[end], strlen(packet_str[end]), 0);	
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			printf("\"%s\" is transmitted.\n", packet_str[end]);		

			for(int j=0; j<=end; j++){		
				if(ackCheck[j]==1){		
					continue;
				}
				count[j]++;
				if(count[j]>4){
					printf("\"%s\" is timeout.\n", packet_str[j]);		
					escape = 1;
					break;
				}
			}

			if(escape == 1){		
				break;
			}

		}else{		
			printf("\"%s\" is received and ignored.\n", ack);	
			end = ackCount+1;		 
			for(int j=0; j<=end; j++){		
				if(ackCheck[j]==1){		
					continue;
				}
				count[j]++;
				if(count[j]>4){
					printf("\"%s\" is timeout.\n", packet_str[j]);
					escape = 1;
					for (int p = j; p < sizeof(count) / sizeof(count[0]); p++) {
    					count[p] = 0;
					}
					break;
				}
			}

			if(escape == 1){		
				break;
			}
		}
	}
	
	end--;
	
	for(int i=0; i<WINDOW_SIZE; i++){
		end++;
		retval = send(sock, packet_str[end], strlen(packet_str[end]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		for(int j=0; j<=end; j++){
			if(ackCheck[j]==1){		
				continue;
			}
			count[j]++;
			if(count[j]>4){
				printf("\"%s\" is timeout.\n", packet_str[j]);
				break;
			}
		}
		
		printf("\"%s\" is transmitted.\n", packet_str[end]);		//한번 보낼때마다 출력함
	}
	
	// 소켓 닫기
	sleep(1);
	close(sock);
	return 0;
}
