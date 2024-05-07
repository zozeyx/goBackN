#include "Common.h"

#define SERVERPORT 9000 
#define BUFSIZE    512  
#define PACKET_SIZE 8

int seq_num = -1; 
char packet_str[9][20];
int packet[9] = {0,1,2,3,4,5,6,7,8};

int recvlen(int sockfd, char *buf, size_t len, int flags) { //데이터 길이 반환
	int data_len = 0;
	int ret;
	while(data_len < len) {
		ret = recv(sockfd, &buf[data_len], len - data_len, flags);
		if (ret == 0) { 
			return ret;
		} else {
			data_len += ret;
		}
	}
	return data_len;
}

int main(int argc, char *argv[]){

	int retval;
	
	for (int i = 0; i < 9; i++) {	//packet0, packet1, packet2 ... 저장
        sprintf(packet_str[i], "packet %d", packet[i]);
    }


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
		if (client_sock == INVALID_SOCKET) { 
			err_display("accept()");
			break;
		}

		for(int i=0; i<4; i++){ //처음에 패킷 4개받기
			
			char data[BUFSIZE + 1];

        	retval = recvlen(client_sock, data, PACKET_SIZE, 0);
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()"); 
        		break;
    		} else if (retval == 0) {
        		break;        		
			}
			seq_num++;		

			if(strncmp(data, packet_str[2], PACKET_SIZE) == 0){		//packet2 드랍 가정
				seq_num--;		
				continue;		
			}

			if (strncmp(data, packet_str[seq_num], PACKET_SIZE) != 0) {		
				printf("\"%s\" is received and dropped.", data); 	

				seq_num = seq_num - 1;	
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", seq_num);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", seq_num); 

    		}else{		//정상인 경우
				printf("\"%s\" is received.", data);
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
			char data[BUFSIZE + 1];

        	retval = recvlen(client_sock, data, PACKET_SIZE, 0); 
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()");  
        		break;
    		} else if (retval == 0) { 
        		break;        		
			}
			seq_num++;	

			if (strncmp(data, packet_str[seq_num], PACKET_SIZE) != 0) {
				printf("\"%s\" is received and dropped.", data); 	

				seq_num = seq_num - 1;	
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", seq_num);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", seq_num); 
			}

		}
		
		for(int i=0; i<4; i++){
			char data[BUFSIZE + 1]; 

        	retval = recvlen(client_sock, data, PACKET_SIZE, 0); 
        	if (retval == SOCKET_ERROR) {
            	err_display("recv()"); 
        		break;
    		} else if (retval == 0) {
        		break;        		
			}
			seq_num++;	

			if (strncmp(data, packet_str[seq_num], PACKET_SIZE) != 0) {		
				printf("\"%s\" is received and dropped.", data); 	

				seq_num = seq_num - 1;	
				
				char ack_bufTemp[BUFSIZE];
				
				sprintf(ack_bufTemp, "ACK %d", seq_num);
				retval = send(client_sock, ack_bufTemp, strlen(ack_bufTemp), 0);  
            	if (retval == SOCKET_ERROR) {   
                	err_display("send()");
                	break;
            	}
				
            	printf("\"ACK %d\" is transmitted.\n", seq_num); 

    		}else{		//정상인 경우
				printf("\"%s\" is received and delivered.", data);
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
		
		

		// 소켓 닫기
		close(client_sock);
		break;
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}
