#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define KEY_SIZE 16
#define VALUE_SIZE 64
#define MAX_KEY 100000

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

int compare(const void *a, const void *b) { 
	if (*(int *)a < *(int *)b) 
		return -1; 
	if (*(int *)a > *(int *)b) 
		return 1; 
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3){
		printf("Usage: ./client WRatio N\n");
	  return 1;
	}
	int wratio_req = atoi(argv[1]);
	int total_req = atoi(argv[2]);

	int SERVER_PORT = 5001;
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

	//헤더 구조체 선언
	struct kvs_hdr my_hdr;

	//각 테스트의 latency를 담을 배열 선언
	uint64_t latcy[total_req]; 

	for (int i = 0; i < total_req; i++){ //전체 요청 개수만큼 반복
		//헤더 구조체 초기화
		memset(&my_hdr, 0, sizeof(struct kvs_hdr)); 
		my_hdr.op = -1;

		//클라이언트에서 서버로 메시지 전송
		int isWrite = (rand() % 100) < wratio_req;
		int rdm_key = rand() % (MAX_KEY + 1);
		
		if(!isWrite){
			my_hdr.op = 0; //OP_READ
			sprintf(my_hdr.key, "%d", rdm_key);
		}
		else {
			my_hdr.op = 2; //OP_WRITE
			sprintf(my_hdr.key, "%d", rdm_key);
			strcpy(my_hdr.value, "DCBADCBADBCA\0");
		}		
		my_hdr.latency = get_cur_ns();
		sendto(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
	
		//서버로부터 메시지 수신
		n = recvfrom(sock, &my_hdr, 
			sizeof(struct kvs_hdr), 0, NULL, NULL);
		if (n > 0) {
			if(my_hdr.op == 1){ //OP_READ_REPLY 
				// printf("The key %s has value %s\n", my_hdr.key, my_hdr.value);
			}
			else if(my_hdr.op == 3){ //OP_WRITE_REPLY
				// printf("your write for %s is done\n", my_hdr.key);
			}
		}
		my_hdr.latency = get_cur_ns() - my_hdr.latency;
		latcy[i] = my_hdr.latency / 1000; 
	}

	//평균 latency 구하기
	uint64_t latcy_sum = 0;
	for(int i = 0; i < total_req; i++)
		latcy_sum += latcy[i];
	printf("Average Latency: %lu microseconds\n", 
		latcy_sum / total_req);
	
	//99% 꼬리 latency 구하기
	int tail_idx = total_req * 0.99;
	qsort(latcy, total_req, sizeof(uint64_t), compare);
	printf("99per tail Latency: %lu microseconds\n", 
		latcy[tail_idx]);
	
	close(sock);
	return 0;
}