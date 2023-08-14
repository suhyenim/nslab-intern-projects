#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define KEY_SIZE 16
#define VALUE_SIZE 64

#pragma pack(1) // padding 방지
struct kvs_hdr{
  uint32_t op; // Operation type. OP_READ는 0, OP_READ_REPLY는 1, OP_WRITE는 2, OP_WRITE_REPLY는 3으로 상수 정의
  char key[KEY_SIZE]; // KEY_SIZE는 16바이트로 별도 상수 정의
  char value[VALUE_SIZE]; // VALUE_SIZE는 64바이트로 별도 상수 정의
  uint64_t latency; // latency 측정용
} __attribute__((packed)); // padding 방지

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
	// char RecvBuffer[maxlen];
  // char SendBuffer[maxlen];

	//구조체 선언
	struct kvs_hdr my_hdr;

	while (1) {		

		//구조체 초기화
		memset(&my_hdr, 0, sizeof(struct kvs_hdr));
		my_hdr.op = -1;

		//클라이언트에서 받아온 값 가공해서 클라이언트로 전송
		n = recvfrom(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
		if (n > 0) {		
			if(my_hdr.op == 0){
				strcpy(my_hdr.value, "ABCDABCDABCDABCD");
				my_hdr.op = 1; //OP_READ_REPLY
			}
			else if(my_hdr.op == 2){
				strcpy(my_hdr.value, "");
				my_hdr.op = 3; //OP_WRITE_REPLY
			}
			sendto(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
		}
	}
	close(sock);

	return 0;
}