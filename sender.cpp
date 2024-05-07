#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1"; 

#define SERVERPORT 9000     
#define BUFSIZE    512      
#define ACK_SIZE	5

int packet[9] = {0,1,2,3,4,5,6,7,8};	//패킷선언
char packet_str[9][20];
int sender_window = 4;	//한번에 응답없이 보낼 수 있는 패킷 개수
int h = 0;
int t = -1;
int ackCount = -1;	//받은 ack의 숫자. 하나씩 받을 때 마다 1개씩 증가한다. packet 0에 대한 ack을 받았으면 ackCount = 0으로 증가
char ack[100];
int timeCount[9] = {0};
int ackRecvCheck[9] = {0};	//보낸 패킷에 대한 ack을 받았는지 확인. 받았으면 1로 표시.
int breakCon = 0;	//반복문을 빠져나올 스위치. 1이면 빠져나온다.

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

int main(int argc, char *argv[]){

	for (int i = 0; i < 9; i++) {	//packet_str에 packet 0 이런식으로 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }
	//printf("%s", packet_str[0]);

	int retval;

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

	//우선 먼저 4개를 보냄
	for(int i=0; i<sender_window; i++){		//4개를 보내는데 보낼때 마다 각 패킷의 timeCount++
		t++;
		retval = send(sock, packet_str[t], strlen(packet_str[t]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		for(int j=0; j<=t; j++){
			if(ackRecvCheck[j]==1){		//ack를 받았으면 그에 해당하는 timeCount는 증가하지 않도록 한다.
				continue;
			}
			timeCount[j]++;
			if(timeCount[j]>4){
				printf("\"%s\" is timeout.\n", packet_str[j]);
				break;
			}
		}
		/*
		timeCount[t]++;
		timeCount[t-1]++; timeCount[t]++;
		timeCount[t-2]++; timeCount[t-1]++; timeCount[t]++;
		timeCount[t-3]++; timeCount[t-2]++; timeCount[t-1]++; timeCount[t]++;
		*/
		printf("\"%s\" is transmitted.\n", packet_str[t]);		//한번 보낼때마다 출력함
	}	

	for(int i=0; i<5; i++){
		// 데이터 받기
		char buf[BUFSIZE + 1];

		retval = recv_fixlen(sock, buf, ACK_SIZE, 0);  	//상대가 보낸 데이터 ack을 버퍼에 저장
		if (retval == SOCKET_ERROR) {   
			err_display("recv()");
			break;
		} else if (retval == 0) {   
			break;
		} else if (strncmp(buf, ack, ACK_SIZE) != 0) {	//ack이 중복이 아니라면, 수신한 데이터와 ack 변수의 값이 다르다면 ack 변수에 수신한 데이터를 할당
			strcpy(ack, buf);	//ack에 받은 데이터를 저장
			printf("\"%s\" is received.", ack);	
			ackCount++;
			ackRecvCheck[ackCount] = 1;
			h++;	//head 포인터 증가 - packet배열의 포인터이다.
			t++;	//tail포인터 증가 - packet배열의 다음 칸을 보낼 수 있다.

			retval = send(sock, packet_str[t], strlen(packet_str[t]), 0);	//	무사히 ack을 받았다면 다음 패킷을 보낸다.
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			printf("\"%s\" is transmitted.\n", packet_str[t]);		// 패킷을 보내고 출력

			for(int j=0; j<=t; j++){		//ackRecvCheck배열을 현재 패킷 배열을 가리키는 tail포인터가 위치하는 곳 까지 돌면서 받은 위치가 있는지 확인한다. (1인 부분)
				if(ackRecvCheck[j]==1){		//ack를 받았으면 그에 해당하는 timeCount는 증가하지 않도록 한다.
					continue;
				}
				timeCount[j]++;
				if(timeCount[j]>4){
					printf("\"%s\" is timeout.\n", packet_str[j]);		//만약 timeout된 패킷이 있다면 출력하고 breakCon을 1로 만든다 그리고 해당 for문을 빠져나온다.
					breakCon = 1;
					break;
				}
			}

			if(breakCon == 1){		//위에서 timeout이 발생하였기 때문에 큰 while문을 빠져나간다.
				break;
			}

		}else{		//만약 ack가 중복이라면 밑의 내용을 출력하고 패킷 배열을 가리키는 t포인터에 값을 설정해준다.
			printf("\"%s\" is received and ignored.\n", ack);	//만약 수신한 데이터와 ack변수가 같다면 중복이므로 이 내용을 출력하고
			t = ackCount+1;		//t값을 설정해주어서 타임아웃 이후에 다시 보낼 패킷의 위치를 지정한다. 근데 1을 더하는게 아니라 보낼때 애초에 t++을 하고 보내게끔 코드를 짜놔서 패킷 2부터 보내야하지만 그보다 하나 작은 1로 설정되게한다.

			//중복일때도 timeCount는 증가시킨다. 왜냐? 원래는 ack받고 패킷보내야 하는데 보낼때마다 timeCount를 증가시키는거고 근데 중복이라서 받기만 하고 보내지는 않는다. 따라서 timeCount는 증가시키는게 맞다. 포함이 되기 때문에.
			//보내지만 않을뿐 보내는것과 시간은 동일하게 흘러간다.
			for(int j=0; j<=t; j++){		
				if(ackRecvCheck[j]==1){		
					continue;
				}
				timeCount[j]++;
				if(timeCount[j]>4){
					printf("\"%s\" is timeout.\n", packet_str[j]);		//만약 timeout된 패킷이 있다면 출력하고 breakCon을 1로 만든다 그리고 해당 for문을 빠져나온다.
					breakCon = 1;
					for (int p = j; p < sizeof(timeCount) / sizeof(timeCount[0]); p++) {
    					timeCount[p] = 0;
					}
					break;
				}
			}

			if(breakCon == 1){		//위에서 timeout이 발생하였기 때문에 큰 while문을 빠져나간다.
				break;
			}
		}
	}
	/*
	for (int i = 0; i < 9; ++i) {
        printf("timeCount[%d] = %d\n", i, timeCount[i]);
		printf("ackRecv[%d] = {%d}\n", i, ackRecvCheck[i]);
		printf("----------------------\n");
    }
	*/
	
	t--;
	
	for(int i=0; i<sender_window; i++){
		t++;
		retval = send(sock, packet_str[t], strlen(packet_str[t]), 0);
    	if (retval == SOCKET_ERROR) {
   			err_display("send()");
        	break;
    	}

		for(int j=0; j<=t; j++){
			if(ackRecvCheck[j]==1){		//ack를 받았으면 그에 해당하는 timeCount는 증가하지 않도록 한다.
				continue;
			}
			timeCount[j]++;
			if(timeCount[j]>4){
				printf("\"%s\" is timeout.\n", packet_str[j]);
				break;
			}
		}
		
		printf("\"%s\" is transmitted.\n", packet_str[t]);		//한번 보낼때마다 출력함
	}
	
	// 소켓 닫기
	sleep(1);
	close(sock);
	return 0;
}
