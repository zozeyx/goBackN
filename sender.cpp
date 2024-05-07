#include "Common.h"
#include <cstring>

#define SERVERPORT 9000 
#define BUFSIZE    512  
#define PACKET_SIZE 8

int seq_num = -1; 
char packet_str[9][20];
int packet[9] = {0,1,2,3,4,5,6,7,8};
char tempArr[9][20];

int recvlen(int sockfd, char *buf, size_t len, int flags) { //데이터 길이 반환
	int data_len = 0;
	int ret;
	while(data_len < len) {
		ret = recv(sockfd, &buf[data_len], len - data_len, flags);
		if (ret == SOCKET_ERROR) {
			return ret;
		} else if (ret == 0) { // 클라이언트가 연결을 종료한 경우 반복문 종료
			return ret;
		} else {
			data_len += ret;
			buf[data_len] = '\0';
		}
	}
	return data_len;
}

int main(int argc, char *argv[]) {

	int retval;
	int start = 0;
	int end = 0;

	for (int i = 0; i < 9; i++) {	//packet0, packet1, packet2 ... 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }

	
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);   
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");  

	
    struct sockaddr_in serveraddr;     
	memset(&serveraddr, 0, sizeof(serveraddr));     
	serveraddr.sin_family = AF_INET;    
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);     
	serveraddr.sin_port = htons(SERVERPORT);    
    
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));  
	if (retval == SOCKET_ERROR) err_quit("bind()");  

	
	retval = listen(listen_sock, SOMAXCONN);   
	if (retval == SOCKET_ERROR) err_quit("listen()");  

	
	SOCKET client_sock;   
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		
		addrlen = sizeof(clientaddr);   
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen); 
		if (client_sock == INVALID_SOCKET) {   
			err_display("accept()");
			break;
		}

		for(int i=0; i<4; i++){ //처음에 패킷 4개받기
			char received_data[BUFSIZE + 1]; 

        	retval = recvlen(client_sock, received_data, PACKET_SIZE, 0); 
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  
        		break;
    		} else if (retval == 0) {
        		break;        		
			}
			seq_num++;

			if(strncmp(received_data, packet_str[2], PACKET_SIZE) == 0){		//packet2 드랍 가정
				seq_num--;
				continue;		
			}

			if (strncmp(received_data, packet_str[seq_num], PACKET_SIZE) != 0) {		
				printf("\"%s\" is received and buffered.\n", received_data); 	

				seq_num = seq_num + 1;
				start = seq_num - 1;

				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", seq_num);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", seq_num);
				strcpy(tempArr[seq_num], received_data);

    		}else{ 		//정상인 경우
				printf("\"%s\" is received.\n", received_data);
				char ack_buf[BUFSIZE];

				sprintf(ack_buf, "ACK %d", seq_num); 

    	        
     	       	retval = send(client_sock, ack_buf, strlen(ack_buf), 0);  
      	    	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
            	printf("\"ACK %d\" is transmitted.\n", seq_num); 
			}
		}		
		
		for(int i=0; i<2; i++){
			char received_data[BUFSIZE + 1]; 

        	retval = recvlen(client_sock, received_data, PACKET_SIZE, 0); 
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  
        		break;
    		} else if (retval == 0) { 
        		break;        		
			}
			seq_num++;
			strcpy(tempArr[seq_num], received_data);
			printf("\"%s\" is received and buffered.\n", received_data); 	
			printf("\"ACK %d\" is transmitted.\n", seq_num); 
			end = seq_num;

		}
		
		for(int i=0; i<4; i++){
			char received_data[BUFSIZE + 1];

        	retval = recvlen(client_sock, received_data, PACKET_SIZE, 0); 
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()"); 
        		break;
    		} else if (retval == 0) { 
        		break;        		
			}
			printf("\"%s\" is received.", received_data); 	
			
			strcpy(tempArr[start], received_data);

			for(int p=start; p<end; p++){
				printf("%s, ", tempArr[p]);
			}
			printf("and %s are delivered.",tempArr[end]);

				char ack_bufTemp[BUFSIZE];
				sprintf(ack_bufTemp, "ACK %d", start);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
            	printf("\"ACK %d\" is transmitted.\n", start); 

		}
		
		

		// 소켓 닫기
		close(client_sock);
		break;
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
