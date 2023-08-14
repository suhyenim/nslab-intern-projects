#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
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

uint64_t get_cur_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  uint64_t t = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
  return t;
}

int readline ( int fd, char *ptr, int maxlen ){
   int n, rc;
   char c;
   for(n=1; n<maxlen; n++){
      if ( (rc = read(fd, &c, 1)) == 1 ){
         *ptr ++ = c;
         if ( c == '\n' ) break;
      } else if ( rc == 0 ){
         if ( n == 1 ) return 0;
         else break;
      }
   }
   *ptr = 0;
   return n;
}

int main(int argc, char *argv[]) {
	if ( argc < 2 ){
	 printf("Input : %s port number\n", argv[0]);
	 return 1;
	}

	int SERVER_PORT = atoi(argv[1]);
	const char* server_name = "localhost"; // 127.0.0.1
	struct sockaddr_in srv_addr; // Create socket structure
	memset(&srv_addr, 0, sizeof(srv_addr)); // Initialize memory space with zeros
	srv_addr.sin_family = AF_INET; // IPv4
	srv_addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, server_name, &srv_addr.sin_addr);  // Convert IP addr. to binary

	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Could not create socket\n");
		exit(1);
	}

  struct sockaddr_in cli_addr;
  int cli_addr_len = sizeof(cli_addr);

	int n = 0;
	int maxlen = 1024;
	char RecvBuffer[maxlen];
  char SendBuffer[maxlen];

	//구조체 선언
	struct kvs_hdr my_hdr;

	while(1){
		//버퍼 초기화
		memset(SendBuffer, 0, sizeof(SendBuffer));

		//구조체 초기화
		memset(&my_hdr, 0, sizeof(struct kvs_hdr));
		my_hdr.op = -1;

		if (readline(0, SendBuffer, maxlen) > 0 ){
			
			//마지막 문자를 개행문자에서 널문자로 변경
			int len = (int)strlen(SendBuffer); //버퍼 안 문자열의 길이
			if (SendBuffer[len-1]== '\n')
				SendBuffer[len-1]='\0'; 
			if (strlen(SendBuffer) == 0)
				break;

			//띄어쓰기 처리 
			int count_space = 0;
			for (int i = 0; i < len; i++) {
				if (SendBuffer[i] == ' '){
					if (SendBuffer[i+1] == '\0' || SendBuffer[i+1] == ' '){
						count_space = -1;
						break;
					}
					count_space++;
				}	
			}

      //클라이언트에서 서버로 전송
			if(strncmp(SendBuffer, "get ", 4) == 0 && count_space == 1){
				my_hdr.op = 0; //OP_READ
				strcpy(my_hdr.key, SendBuffer + 4);
			}
			else if(strncmp(SendBuffer, "put ", 4) == 0 && count_space == 2){
				my_hdr.op = 2; //OP_WRITE
				strcpy(my_hdr.key, strtok(SendBuffer + 4, " "));
				strcpy(my_hdr.value, strtok(NULL, " "));
			}						
			my_hdr.latency = get_cur_ns();
			sendto(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
			
			//서버에서 받아온 값 출력
			n = recvfrom(sock, &my_hdr, sizeof(struct kvs_hdr), 0, NULL, NULL);
			if (n > 0) {
				if(my_hdr.op == 1){ //OP_READ_REPLY 
					printf("The key %s has value %s\n", my_hdr.key, my_hdr.value);
				}
				else if(my_hdr.op == 3){ //OP_WRITE_REPLY
					printf("your write for %s is done\n", my_hdr.key);
				}
			}
			my_hdr.latency = get_cur_ns() - my_hdr.latency;
      printf("Latency: %lu microseconds\n", my_hdr.latency / 1000);
		}
	}

	close(sock);

	return 0;
}
