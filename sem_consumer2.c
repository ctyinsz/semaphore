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

//线程函数  
void *thread_func(void *); 
void sigchld_handler( int  ) ;
struct share_msg *msg = NULL;
//sem_t *rsem;
//sem_t *wsem;

int main()  
{ 
  int res = -1,i,j;
  pthread_t thread_c;  
  void *thread_result = NULL; 
	void *shm = NULL;//分配的共享内存的原始首地址
	int shmid;//共享内存标识符
	//创建共享内存
	shmid = shmget((key_t)SHKEY, sizeof(struct share_msg), 0666|IPC_CREAT);
	if(shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}	
	//将共享内存连接到当前进程的地址空间
	shm = shmat(shmid, 0, 0);
	if(shm == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("\nMemory attached at %X\n", (int)shm);	
	//设置共享内存
	msg = (struct share_msg*)shm;

    //初始化信号量,初始值为0  
//    rsem = sem_open(SEM_NAME_R,O_CREAT,0644,0);
//    res = sem_init(&msg->rsem, 0, 0);  
//    if(res == -1)  
//    {  
//        perror("semaphore intitialization failed\n");  
//        exit(EXIT_FAILURE);  
//    }  
    //初始化信号量,初始值为1 
//    wsem = sem_open(SEM_NAME_W,O_CREAT,0644,MSG_NUM); 
//    res = sem_init(&msg->wsem, 0, MSG_NUM);  
//    if( res == -1)  
//    {  
//        perror("semaphore intitialization failed\n");  
//        exit(EXIT_FAILURE);  
//    } 
    
    //在主线程中屏蔽SIGINT信号，以防被子线程继承
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
        
    //创建消费者线程，并把msg作为线程函数的参数  
    res = pthread_create(&thread_c, NULL, thread_func, NULL);  
    if(res != 0)  
    {
        perror("consumer thread_create failed\n");  
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
		res = pthread_cancel(thread_c);
    if(res != 0)  
    {  
        perror("pthread_cancel failed\n");  
        exit(EXIT_FAILURE);  
    }	
    //等待子线程结束    
    res = pthread_join(thread_c, &thread_result);  
    if(res != 0)  
    {  
        perror("pthread_join failed\n");  
        exit(EXIT_FAILURE);  
    } 
    printf("Thread joined\n"); 
    
    //清理信号量  
//    sem_close(&msg->rsem);  
//    sem_close(&msg->wsem); 	
	//把共享内存从当前进程中分离
	if(shmdt(shm) == -1)
	{
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}	
	exit(EXIT_SUCCESS); 
}

void* thread_func(void *msgs)  
{
    char buf[BUF_SIZE]={0};
    //把信号量减1  
    sem_wait(&msg->rsem); 
    int ret = dataget(buf);
//    printf("buf: %s\n", buf);
//    while(strcmp("end\n", buf) != 0)  
    while(1)  
    {  
        int i = 0,j=0;  
        //把小写字母变成大写
        if(!ret)
        {  
	        for(; buf[i] != '\0'; ++i)  
	        {  
	            if(buf[i] >= 'a' && buf[i] <= 'z')  
	            {  
	                buf[i] -= 'a' - 'A';  
	            }  
	        }  
	        printf("You input %d characters\n", i-1);  
	        printf("To Uppercase: %s\n", buf); 
	        for(i=0;i<2;i++)
	        	for(j=0;j<100000000;j++);
	        //把信号量加1，表明子线程处理完成  
	        sem_post(&msg->wsem);
        }
        //把信号量减1 ,等待生产者 
        sem_wait(&msg->rsem);
        memset(buf,0x00,sizeof(buf));
        ret = dataget(buf);
    }
    sem_post(&msg->wsem);  
//    raise(SIGINT);
    //退出线程  
    pthread_exit(NULL);  
} 
//int datapull(char * str)
//{
//	
//	if(msg->count>=MSG_NUM)
//	{
//		printf("Queue is full\n");
//		return 1;
//	}
//	else
//	{
//		msg->count++;
//		if(msg->widx >= MSG_NUM)
//			msg->widx = 0;
//	}
//	strcpy(msg->smsg[msg->widx],str);
//	msg->widx++;
//	printf("push:%s,idx:%d\n",str,msg->widx-1);
//	return 0;
//}
int dataget(char * str)
{
	if(msg->count<=0)
	{
		printf("Queue is empty\n");
		return 1;
	}
	else
	{
		msg->count--;
		if(msg->ridx >= MSG_NUM)
			msg->ridx = 0;
	}
	strcpy(str,msg->smsg[msg->ridx]);
	memset(msg->smsg[msg->ridx],0x00,sizeof(msg->smsg[msg->ridx]));
	msg->ridx++;
	printf("pop:%s,idx:%d\n",str,msg->ridx-1);
	return 0;
}

void sigchld_handler( int signo ){
	printf("sigchld_handler\n");
	if (signo == SIGINT) 
	{ 
		printf("get SIGINT\n"); 
//    printf("sigchld_handler\n");
  }
//    pthread_cancel();
//    sem_destroy(&msg->rsem);  
//    sem_destroy(&msg->wsem); 
    return;
}

