#include "pipe_header.h"

#define FIFO_FILE1 "./fifo1" // 클라이언트1로부터 메세지를 수신하는 파이프
#define FIFO_FILE2 "./fifo2" // 클라이언트1에게 메세지를 전송하는 파이프
#define FIFO_FILE3 "./fifo3" // 클라이언트2로부터 메세지를 수신하는 파이프
#define FIFO_FILE4 "./fifo4" // 클라이언트2에게 메세지를 전송하는 파이프
#define THREAD_NUM 4
#define BILLION 1000000000L;

// 경쟁 상태를 방지하기위한 mutex lock 변수와 동기화를 위한 조건 변수입니다.
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_cond_t cond1=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2=PTHREAD_COND_INITIALIZER;

char buf1[BUF_SIZE];
char buf2[BUF_SIZE];
int log_txt;
int stop_cnt=0;
int fd1,fd2,fd3,fd4;

// 성능 측정을 위한 변수입니다.
int time_cnt=0;
struct timespec start,stop;

// 클라이언트1로부터 수신하는 함수입니다.
void *receive1(void)
{
	time_t tm_time;
	struct tm* st_time;
	char time_buf[BUF_SIZE+30]="-1";
	char tmp[BUF_SIZE]="-1";
	int time_cnt=0;
	double accum;

	// 파이프를 생성하고 fd1과 연결하는 부분입니다.
	if(mkfifo(FIFO_FILE1,0666)==-1)
	{
		perror("receive1() mkfifo() error");
		exit(1);
	}

	if((fd1=open(FIFO_FILE1,O_RDONLY))==-1)
	{
		perror("receive1() open() error");
		exit(1);
	}

	while(1)
	{
		pthread_mutex_lock(&mutex1);

		// 클라이언트1과 연결된 파이프로 수신하는 부분입니다.
		if(read(fd1,buf1,BUF_SIZE)==-1) 
		{
			perror("func1() read() error");
			exit(1);
		}

		time_cnt++;

		if(time_cnt==2)
		{
			if(clock_gettime(CLOCK_MONOTONIC,&start)==-1)
			{
				perror("clock gettime error");
			}
		}

		if(time_cnt==11)
		{
			if(clock_gettime(CLOCK_MONOTONIC,&stop)==-1)
			{
				perror("clock gettime error");
			}
			accum=(stop.tv_sec-start.tv_sec)+(double)(stop.tv_nsec-start.tv_nsec)/(double)BILLION;
			printf("%.9f\n",accum);
			time_cnt=2;
		}

		// 수신한 메세지에 시간을 더하는 부분입니다.
		strncpy(tmp,buf1,sizeof(buf1));
		time(&tm_time);
		st_time=localtime(&tm_time);
		strftime(time_buf,16,"%y-%m-%d-%lH-%M",st_time);
		time_buf[strlen(time_buf)]='M';
		time_buf[strlen(time_buf)]='\0';
		strncat(time_buf," - client1 : ",13);
		time_buf[strlen(time_buf)]='\0';
		strncat(time_buf,tmp,sizeof(tmp));
		time_buf[strlen(time_buf)]='\n';

		printf("%s",time_buf);

		// 수신한 메세지를 로그 파일에 저장하는 부분입니다.
		if(write(log_txt,time_buf,strlen(time_buf))==-1)
		{
			perror("func1() write() error1");
			exit(1);
		}

		if(!strcmp(buf1,"!stop"))
		{
			stop_cnt++;
			break;
		}

		memset(time_buf,0,BUF_SIZE+30);

		// send2()을 수행하는 쓰레드와 동기화를 위한 함수입니다.
		pthread_cond_signal(&cond1);
		pthread_mutex_unlock(&mutex1);
		sleep(1);
	}
	pthread_exit(NULL);
	return 0;
}

// 클라이언트2로부터 수신하는 함수입니다.
void *receive2(void)
{
	time_t tm_time;
	struct tm* st_time;
	char time_buf[BUF_SIZE+30]="-1";
	char tmp[BUF_SIZE]="-1";

	// 파이프를 생성하고 fd3과 연결하는 부분입니다.
	if(mkfifo(FIFO_FILE3,0666)==-1)
	{
		perror("receive2() mkfifo() error");
		exit(1);
	}

	if((fd3=open(FIFO_FILE3,O_RDONLY))==-1)
	{
		perror("receive2() open() error");
		exit(1);
	}

	while(1)
	{
		pthread_mutex_lock(&mutex2);

		// 클라이언트2와 연결된 파이프로 수신하는 부분입니다.
		if(read(fd3,buf2,BUF_SIZE)==-1)
		{
			perror("func2() read() error");
			exit(1);
		}

		// 수신한 메세지에 시간을 더하는 부분입니다.
		strncpy(tmp,buf2,sizeof(buf2));
		time(&tm_time);
		st_time=localtime(&tm_time);
		strftime(time_buf,16,"%y-%m-%d-%lH-%Mm",st_time);
		time_buf[strlen(time_buf)]='\0';
		strncat(time_buf," - client2 : ",13);
		time_buf[strlen(time_buf)]='\0';
		strncat(time_buf,tmp,sizeof(tmp));
		time_buf[strlen(time_buf)]='\n';

		printf("%s",time_buf);

		// 수신한 메세지를 로그 파일에 저장하는 부분입니다.
		if(write(log_txt,time_buf,strlen(time_buf))==-1)
		{
			perror("func2() write() error1");
			exit(1);
		}
		
		if(!strcmp(buf2,"!stop"))
		{
			stop_cnt++;
			break;
		}
			
		memset(time_buf,0,BUF_SIZE+30);

		// send1()을 수행하는 쓰레드와 동기화를 위한 함수입니다.
		pthread_cond_signal(&cond2);
		pthread_mutex_unlock(&mutex2);
		sleep(1);
	}
	pthread_exit(NULL);
	return 0;
}

// 클라이언트2에서 수신한 메세지를 클라이언트1에게 보내는 함수입니다.
void *send1(void)
{
	if(mkfifo(FIFO_FILE2,0666)==-1)
	{
		perror("send1() mkfifo() error");
		exit(1);
	}

	if((fd2=open(FIFO_FILE2,O_WRONLY))==-1)
	{
		perror("send1() open() error");
		exit(1);
	}

	while(1)
	{
		// receive1()를 수행하는 쓰레드와 동기화를 위한 부분입니다.
		pthread_mutex_lock(&mutex2);
		pthread_cond_wait(&cond2,&mutex2);

		// 클라이언트1과 연결된 파이프에 보내는 부분입니다.
		if(write(fd2,buf2,BUF_SIZE)==-1)
		{
			perror("send1() write() error");
			exit(1);
		}
		pthread_mutex_unlock(&mutex2);

		if(stop_cnt==2)
			break;
	}
	pthread_exit(NULL);
	return 0;
}

// 클라이언트1에서 수신한 메세지를 클라이언트2에게 보내는 함수입니다.
void *send2(void)
{
	if(mkfifo(FIFO_FILE4,0666)==-1)
	{
		perror("send2() mkfifo() error");
		exit(1);
	}

	if((fd4=open(FIFO_FILE4,O_WRONLY))==-1)
	{
		perror("send2() open() error");
		exit(1);
	}

	while(1)
	{
		// receive2()를 수행하는 쓰레드와 동기화를 위한 부분입니다.
		pthread_mutex_lock(&mutex1);
		pthread_cond_wait(&cond1,&mutex1);

		// 클라이언트2와 연결된 파이프에 보내는 부분입니다.
		if(write(fd4,buf1,BUF_SIZE)==-1)
		{
			perror("send1() write() error");
			exit(1);
		}
		pthread_mutex_unlock(&mutex1);

		if(stop_cnt==2)
			break;
	}
	pthread_exit(NULL);
	return 0;
}


int main(void)
{
	pthread_t pthread[THREAD_NUM];
	int status,i;

	pthread_mutex_init(&mutex1,NULL);
	pthread_mutex_init(&mutex2,NULL);

	// 기존에 named pipe가 있으면 삭제하는 부분입니다.
	if(access(FIFO_FILE1,F_OK)==0)
		unlink(FIFO_FILE1);
	if(access(FIFO_FILE2,F_OK)==0)
		unlink(FIFO_FILE2);

	if(access(FIFO_FILE3,F_OK)==0)
		unlink(FIFO_FILE3);

	if(access(FIFO_FILE4,F_OK)==0)
		unlink(FIFO_FILE4);

	if((log_txt=open("pipe_log.txt",O_RDWR | O_TRUNC,O_APPEND))==-1)
	{
		perror("func3() open() error");
		exit(1);
	}

	if(pthread_create(&pthread[0],NULL,(void*)receive1,NULL)<0)
	{
		perror("pthread[0] create error");
		exit(1);
	}

	if(pthread_create(&pthread[1],NULL,(void*)receive2,NULL)<0)
	{
		perror("pthread[1] create error");
		exit(1);
	}

	if(pthread_create(&pthread[2],NULL,(void*)send1,NULL)<0)
	{
		perror("pthread[2] create error");
		exit(1);
	}

	if(pthread_create(&pthread[3],NULL,(void*)send2,NULL)<0)
	{
		perror("pthread[3] create error");
		exit(1);
	}

	// receive1과 receive2의 종료를 대기하는 부분입니다.
	pthread_join(pthread[0],NULL);
	printf("클라이언트1 접속 종료\n");
	pthread_join(pthread[1],NULL);
	printf("클라이언트2 접속 종료\n");

	
	// 사용된 mutex lock과 조건 변수를 제거하고 사용한 fd1, fd2를 닫는 부분입니다.
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);

	close(fd1);
	close(fd2);
	close(log_txt);

	return 0;
}


