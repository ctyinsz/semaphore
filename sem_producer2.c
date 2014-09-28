/*
生产者进程：负责向消息队列中写入待处理数据，以及退出进程时回收线程
使用无名信号量做进程和线程间的共享内存同步
sem_init()
sem_destroy()

*/

#include <unistd.h>  
#include <pthread.h>  
#include <semaphore.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <string.h>  
#include <signal.h>
#include <sys/shm.h>
#include <fcntl.h>
#include "shmdata.h"

void *thread_producer(void *);
void sigchld_handler( int  ) ;
struct share_msg *msg = NULL;
//sem_t *rsem;
//sem_t *wsem;

int main()  
{  
	int res = -1,i,j;
	pthread_t thread_p;
	void *thread_result = NULL; 
	
	/********设置共享内存********/
	
	void *shm = NULL;
	int shmid;
	//创建共享内存
	shmid = shmget((key_t)SHKEY, sizeof(struct share_msg), 0666|IPC_CREAT);
	if(shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	//将共享内存连接到当前进程的地址空间
	shm = shmat(shmid, (void*)0, 0);
	if(shm == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory attached at %X\n", (int)shm);
	//设置共享内存
	msg = (struct share_msg*)shm;
	 
  //初始化结构体
  msg->ridx = 0;
  msg->ridx = 0;
  msg->count = 0;
  for(i=0;i<MSG_NUM;i++)
  	memset(msg->smsg[i],0x00,sizeof(msg->smsg[i]));
  	
  /********设置共享内存********/  
  
   /********设置队列操作信号量********/ 
   
    //初始化信号量,初始值为0  
//    rsem = sem_open(SEM_NAME_R,O_CREAT,0644,0);
    res = sem_init(&msg->rsem, 1, 0);  
    if(res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }  
    //初始化信号量,初始值为1 
//    wsem = sem_open(SEM_NAME_W,O_CREAT,0644,MSG_NUM); 
    res = sem_init(&msg->wsem, 1, MSG_NUM);  
    if( res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }
    
    /********设置队列操作信号量********/ 
     
     //在主线程中屏蔽SIGINT信号，以防被子线程继承
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);   
    //创建生产者线程，并把msg作为线程函数的参数  
    res = pthread_create(&thread_p, NULL, thread_producer, NULL);  
    if(res != 0)  
    {
        perror("producer thread_create failed\n");  
        exit(EXIT_FAILURE);  
    }    
    //恢复主线程信号
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    
    //创建信号处理
    struct sigaction s;
    s.sa_handler = sigchld_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);    

    //等待信号  
    //sigsuspend(&sigset);
    pause();    
    //结束子线程		
		res = pthread_cancel(thread_p);    
    if(res != 0)  
    {  
        perror("pthread_cancel failed\n");  
        exit(EXIT_FAILURE);  
    }     
    //等待子线程结束    
    res = pthread_join(thread_p, &thread_result);  
    if(res != 0)  
    {  
        perror("pthread_join failed\n");  
        exit(EXIT_FAILURE);  
    }
    printf("Thread joined\n");  
    //清理信号量  
    sem_destroy(&msg->rsem);  
    sem_destroy(&msg->wsem); 
//    sem_unlink(SEM_NAME_W);
//    sem_unlink(SEM_NAME_R);
		//把共享内存从当前进程中分离
		if(shmdt(shm) == -1)
		{
			fprintf(stderr, "shmdt failed\n");
			exit(EXIT_FAILURE);
		} 
		//删除共享内存
		if(shmctl(shmid, IPC_RMID, 0) == -1)
		{
			fprintf(stderr, "shmctl(IPC_RMID) failed\n");
			exit(EXIT_FAILURE);
		}
    exit(EXIT_SUCCESS);          	
}

/**************************
生产者线程函数
***************************/
void* thread_producer(void *msgs)
{
	char buf[BUF_SIZE]={0};
	printf("Input some text. Enter 'end'to finish...\n"); 
    sem_wait(&msg->wsem);  
//    while(strcmp("end\n", buf) != 0) 
    while(1)  
    {  
        memset(buf,0x00,sizeof(buf));
        fgets(buf, BUF_SIZE, stdin);
        if(strlen(buf)>0)
        {
        	if ( buf[strlen(buf)-1] == '\n')
        		buf[strlen(buf)-1] = 0x00;
      		if(strlen(buf)>0)
      		{
      				datapull(buf);
      				//把信号量加1 ，启动消费者
      				sem_post(&msg->rsem);  
      		}
      		else  
      			continue;     	
        }
        else
        	continue;
        //把sem_add的值减1，即等待子线程处理完成  
        sem_wait(&msg->wsem);  
    }
    pthread_exit(NULL);	
}
/**************************
队列操作函数
***************************/
int datapull(char * str)
{
	
	if(msg->count>=MSG_NUM)
	{
		printf("Queue is full\n");
		return 1;
	}
	else
	{
		msg->count++;
		if(msg->widx >= MSG_NUM)
			msg->widx = 0;
	}
	strcpy(msg->smsg[msg->widx],str);
	msg->widx++;
	printf("push:%s,idx:%d\n",str,msg->widx-1);
	return 0;
}
/**************************
进程终止信号回调函数
***************************/
void sigchld_handler( int signo ){
	printf("sigchld_handler\n");
	if (signo == SIGINT) 
	{ 
		printf("get SIGINT\n"); 
//    printf("sigchld_handler\n");
  }
    return;
}
