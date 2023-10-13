#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define BUFLEN 100
#define TRUE 1
#define FALSE 0

int global_fd;
int quit_chat;
//int begin_chat;

void *read_method(void * arg){
	
	char buf[BUFLEN];
	int ret;
	while(TRUE){
		memset(buf,'\0', sizeof(char)*BUFLEN);
		ret = read(global_fd, buf, BUFLEN);
		if(ret < 0) {
				perror("Chat room full create new room ");
				exit(2);
		}
		
		if(strncmp(buf,"none",4) !=  0)
			printf("%s", buf);
		
		if(quit_chat == 1){
			
			pthread_exit(0);
		}
		//begin_chat=1;
	}

	

	
}


void *write_method(void * arg){
	char buf[BUFLEN];
	sprintf(buf," <- Joined \n");
	
	if(write(global_fd, buf, strlen(buf)+1) < 0) {
			perror("write failed: ");
			exit(3);
	}
		
	
	while(TRUE){

		fgets(buf, BUFLEN, stdin);
		//printf("Sending %s", buf);
		if(write(global_fd, buf, strlen(buf)+1) < 0) {
			perror("write failed: ");
			exit(3);
		}
		if(strncmp(buf,"bye",3)==0){
			quit_chat=1;
			pthread_exit(0);
		}
		
	}
	
}

int main() {

	
	pthread_t read_tid;
	pthread_t write_tid;
	quit_chat = 0;
	//begin_chat = 0;
	global_fd = open("/dev/chat_room", O_RDWR);
	if( global_fd < 0) {
		perror("Open failed: ");
		exit(1);
	}

	pthread_create(&read_tid,NULL,read_method,NULL);
	pthread_create(&write_tid,NULL,write_method,NULL);
	
	
	pthread_join(write_tid,NULL);
	pthread_join(read_tid,NULL);
	
	close(global_fd);
	return 0;
}

