#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

#define KEY_SIZE 16
#define VALUE_SIZE 64

#pragma pack(1) // padding 방지
struct kvs_hdr{
  uint32_t op; // Operation type. OP_READ는 0, OP_READ_REPLY는 1, OP_WRITE는 2, OP_WRITE_REPLY는 3으로 상수 정의
  char key[KEY_SIZE]; // KEY_SIZE는 16바이트로 별도 상수 정의
  char value[VALUE_SIZE]; // VALUE_SIZE는 64바이트로 별도 상수 정의
  uint64_t latency; // latency 측정용
} __attribute__((packed)); // padding 방지

char* get(redisContext *c, int key) {
    redisReply *reply;
    char key_str[KEY_SIZE];
    snprintf(key_str, KEY_SIZE, "%d", key);
    reply = redisCommand(c, "GET %s", key_str);
    if (reply->type == REDIS_REPLY_NIL) {
        //printf("Error: Failed to retrieve value for key: %d\n", key);
        return NULL;
    } else {
        return reply->str;
    }
}
int put(redisContext *c, int key, char *value) {
    redisReply *reply;
    char key_str[KEY_SIZE];
    snprintf(key_str, KEY_SIZE, "%d", key);
    reply = redisCommand(c, "SET %s %s", key_str, value);
    if (reply->type == REDIS_REPLY_ERROR) {
        //printf("Error: Failed to store key-value pair: %s\n", reply->str);
        freeReplyObject(reply);
        return -1;
    } else {
        freeReplyObject(reply);
        return 0;
    }
}

int main(int argc, char *argv[]) {

	// Connect to Redis server
	redisContext *redis_context = redisConnect("127.0.0.1", 6379);
	if (redis_context->err) {
		printf("Failed to connect to Redis: %s\n", redis_context->errstr);
		return 1;
	}

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

	//구조체 선언
	struct kvs_hdr my_hdr;

	while (1) {		
		//구조체 초기화
		memset(&my_hdr, 0, sizeof(struct kvs_hdr));
		my_hdr.op = -1;


		n = recvfrom(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
		if (n > 0) {		
			if(my_hdr.op == 0){
				my_hdr.op = 1; //OP_READ_REPLY
				snprintf(my_hdr.value, VALUE_SIZE, "%.*s", VALUE_SIZE - 1, get(redis_context, atoi(my_hdr.key)));
			}
			else if(my_hdr.op == 2){
				my_hdr.op = 3; //OP_WRITE_REPLY
				put(redis_context, atoi(my_hdr.key), my_hdr.value); 
			}
			sendto(sock, &my_hdr, sizeof(struct kvs_hdr), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
		}
	}
	close(sock);
	redisFree(redis_context);
	return 0;
}