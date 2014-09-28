/*
�����߽��̣���������Ϣ������д����������ݣ��Լ��˳�����ʱ�����߳�
ʹ�������ź��������̺��̼߳�Ĺ����ڴ�ͬ��
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
	
	/********���ù����ڴ�********/
	
	void *shm = NULL;
	int shmid;
	//���������ڴ�
	shmid = shmget((key_t)SHKEY, sizeof(struct share_msg), 0666|IPC_CREAT);
	if(shmid == -1)
	{
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	//�������ڴ����ӵ���ǰ���̵ĵ�ַ�ռ�
	shm = shmat(shmid, (void*)0, 0);
	if(shm == (void*)-1)
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Memory attached at %X\n", (int)shm);
	//���ù����ڴ�
	msg = (struct share_msg*)shm;
	 
  //��ʼ���ṹ��
  msg->ridx = 0;
  msg->ridx = 0;
  msg->count = 0;
  for(i=0;i<MSG_NUM;i++)
  	memset(msg->smsg[i],0x00,sizeof(msg->smsg[i]));
  	
  /********���ù����ڴ�********/  
  
   /********���ö��в����ź���********/ 
   
    //��ʼ���ź���,��ʼֵΪ0  
//    rsem = sem_open(SEM_NAME_R,O_CREAT,0644,0);
    res = sem_init(&msg->rsem, 1, 0);  
    if(res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }  
    //��ʼ���ź���,��ʼֵΪ1 
//    wsem = sem_open(SEM_NAME_W,O_CREAT,0644,MSG_NUM); 
    res = sem_init(&msg->wsem, 1, MSG_NUM);  
    if( res == -1)  
    {  
        perror("semaphore intitialization failed\n");  
        exit(EXIT_FAILURE);  
    }
    
    /********���ö��в����ź���********/ 
     
     //�����߳�������SIGINT�źţ��Է������̼̳߳�
    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);   
    //�����������̣߳�����msg��Ϊ�̺߳����Ĳ���  
    res = pthread_create(&thread_p, NULL, thread_producer, NULL);  
    if(res != 0)  
    {
        perror("producer thread_create failed\n");  
        exit(EXIT_FAILURE);  
    }    
    //�ָ����߳��ź�
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    
    //�����źŴ���
    struct sigaction s;
    s.sa_handler = sigchld_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    sigaction(SIGINT, &s, NULL);    

    //�ȴ��ź�  
    //sigsuspend(&sigset);
    pause();    
    //�������߳�		
		res = pthread_cancel(thread_p);    
    if(res != 0)  
    {  
        perror("pthread_cancel failed\n");  
        exit(EXIT_FAILURE);  
    }     
    //�ȴ����߳̽���    
    res = pthread_join(thread_p, &thread_result);  
    if(res != 0)  
    {  
        perror("pthread_join failed\n");  
        exit(EXIT_FAILURE);  
    }
    printf("Thread joined\n");  
    //�����ź���  
    sem_destroy(&msg->rsem);  
    sem_destroy(&msg->wsem); 
//    sem_unlink(SEM_NAME_W);
//    sem_unlink(SEM_NAME_R);
		//�ѹ����ڴ�ӵ�ǰ�����з���
		if(shmdt(shm) == -1)
		{
			fprintf(stderr, "shmdt failed\n");
			exit(EXIT_FAILURE);
		} 
		//ɾ�������ڴ�
		if(shmctl(shmid, IPC_RMID, 0) == -1)
		{
			fprintf(stderr, "shmctl(IPC_RMID) failed\n");
			exit(EXIT_FAILURE);
		}
    exit(EXIT_SUCCESS);          	
}

/**************************
�������̺߳���
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
      				//���ź�����1 ������������
      				sem_post(&msg->rsem);  
      		}
      		else  
      			continue;     	
        }
        else
        	continue;
        //��sem_add��ֵ��1�����ȴ����̴߳������  
        sem_wait(&msg->wsem);  
    }
    pthread_exit(NULL);	
}
/**************************
���в�������
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
������ֹ�źŻص�����
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
