#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if ( argc < 2 ){
	 printf("Input : %s port number\n", argv[0]);
	 return 1;
	}

	int SERVER_PORT = atoi(argv[1]);

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Could not create listen socket\n");
		exit(1);
	}

	if ((bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0) {
		printf("Could not bind socket\n") ;
		exit(1);
	}

	struct sockaddr_in cli_addr;
  int cli_addr_len = sizeof(cli_addr);

	int  maxlen = 1024;
	int n = 0;
	char RecvBuffer[maxlen];
  char SendBuffer[maxlen];

	while (1) {	
		n = recvfrom(sock, &RecvBuffer, sizeof(RecvBuffer), 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
		if (n > 0) {		
			RecvBuffer[n] = '\0'; // Null-terminate the received string

			//SendBuffer, GPBuffer, XBuffer, YBuffer 버퍼 초기화
			memset(SendBuffer, 0, sizeof(SendBuffer));
			memset(GPBuffer, 0, sizeof(GPBuffer));
			memset(XBuffer, 0, sizeof(XBuffer));
			memset(YBuffer, 0, sizeof(YBuffer));

			//사용할 변수와 버퍼 선언
			int blankX = 0;
			int blankY = 0;
			int startY = 0;
			char GPBuffer[maxlen]; //"get "이나 "put " 담을 버퍼
			char XBuffer[maxlen]; //x값 담을 버퍼
			char YBuffer[maxlen]; //y값 담을 버퍼

			//RecvBuffer에서 get, put, x, y 추출
			for(int i = 0; i < 4; i++){ 
				GPBuffer[i] = (char)RecvBuffer[i]; 
			}	
			for(int i = 4; i < n; i++){ 
				if((char)RecvBuffer[i] == '\n'){
					break;
				}
				if((char)RecvBuffer[i] == ' '){
					blankX = -1;
					startY = i + 1;
					break;
				}
				XBuffer[i - 4] = (char)RecvBuffer[i];
			}
			if(blankX < 0){
				for(int i = startY; i < n; i++){ 
					if((char)RecvBuffer[i] == '\n'){
						break;
					}
					if((char)RecvBuffer[i] == ' '){
						blankY = -1;
						break;
					}
					YBuffer[i - startY] = (char)RecvBuffer[i]; 
				}
			}

			//"get x"인 경우
			if(strcmp(GPBuffer, "get ") == 0 && blankX == 0){
				strcat(SendBuffer, "the value for ");
				strcat(SendBuffer, XBuffer);
				strcat(SendBuffer, " is 0\n");
				printf("%s", SendBuffer);
				sendto(sock, &SendBuffer, sizeof(SendBuffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
			}
			//"put x y"인 경우
			else if(strcmp(GPBuffer, "put ") == 0 && blankX < 0 && blankY == 0){
				strcat(SendBuffer, "your put for ");
				strcat(SendBuffer, XBuffer);
				strcat(SendBuffer, "=");
				strcat(SendBuffer, YBuffer);
				strcat(SendBuffer, " is done!\n");
				printf("%s", SendBuffer);
				sendto(sock, &SendBuffer, sizeof(SendBuffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
			}
			//"get x", "put x y"를 제외한 경우
      else{
				memset(SendBuffer, 0, sizeof(SendBuffer));
				sendto(sock, &SendBuffer, sizeof(SendBuffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
			}
		}
	}
	close(sock);

	return 0;
}