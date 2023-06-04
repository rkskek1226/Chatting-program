#include "pipe_header.h"

#define FIFO_FILE1 "./fifo1" // 서버에게 메세지를 보낼 파이프입니다.
#define FIFO_FILE2 "./fifo2" // 서버로부터 메세지를 받을 파이프입니다.
#define THREAD_NUM 3

char rcv_buf[BUF_SIZE]="-1";
int flag=0;
int stop_flag=0;
int fd1,fd2;

// 메세지를 입력받아 서버에게 보내는 함수입니다.
void *send(void)
{
	char buf[BUF_SIZE]="-1";

	while(1)
	{
		scanf("%s",buf);

		if(strlen(buf)>=30)
		{
			printf("number limit\n");
			continue;
		}

		// 서버와 연결된 파이프를 통해 메세지를 전달하는 부분입니다.
		if(write(fd1,buf,BUF_SIZE)==-1)
		{
			perror("write error");
			exit(1);
		}
		if(!strcmp(buf,"!stop"))
		{
			stop_flag=1;
			break;
		}
	}
	return 0;
}

// 서버로부터 메세지를 받는 함수입니다.
void *receive(void)
{
	char buf[BUF_SIZE]="-1";

	while(1)
	{
		// 서버와 연결된 파이프를 통해 메세지를 받는 부분입니다.
		if(read(fd2,buf,BUF_SIZE)==-1)
		{
			perror("func2() read() error");
			exit(1);
		}

		strncpy(rcv_buf,buf,sizeof(buf));
		memset(buf,0,BUF_SIZE);
		flag=1;

		if(stop_flag==1)
			break;
	}
	pthread_exit(NULL);
	return 0;
}

// 받은 메세지를 출력하는 함수입니다. 
void *print(void)
{
	while(1)
	{
		if(stop_flag==1)
			break;

		if(flag==1)
		{
			printf("receive msg : %s\n",rcv_buf);
			flag=0;
		}
	}
	pthread_exit(NULL);
	return 0;
}


int main(void)
{
	pthread_t pthread[THREAD_NUM];

	// 서버에서 생성하는 파이프를 open합니다.
	if((fd1=open(FIFO_FILE1,O_RDWR))==-1)
	{
		perror("open() error");
		exit(1);
	}

	if((fd2=open(FIFO_FILE2,O_RDWR))==-1)
	{
		perror("open() error");
		exit(1);
	}

	if(pthread_create(&pthread[0],NULL,(void*)send,NULL)<0)
	{
		perror("pthread[0] create error");
		exit(1);
	}

	if(pthread_create(&pthread[1],NULL,(void*)receive,NULL)<0)
	{
		perror("pthread[1] create error");
		exit(1);
	}

	if(pthread_create(&pthread[2],NULL,(void*)print,NULL)<0)
	{
		perror("pthread[2] create error");
		exit(1);
	}

	printf("시작하려면 아무값이나 입력하세요\n");
	pthread_join(pthread[0],NULL); // send()을 수행하는 쓰레드가 종료되기를 대기합니다.
	printf("quit program\n");
	close(fd1);
	close(fd2);
	
	return 0;
}

