#ifndef _SHMDATA_H_HEADER
#define _SHMDATA_H_HEADER

#define BUF_SIZE 512
#define MSG_NUM 3  
#define SHKEY 10086
char SEM_NAME_W[]= "cty_w";
char SEM_NAME_R[]= "cty_r";

struct share_msg
{
//	volatile 	int written;//作为一个标志，非0：表示可读，0表示可写
	char smsg[MSG_NUM][BUF_SIZE];//记录写入和读取的文本
	int ridx;
	int widx;
	int count;
//	sem_t *rsem;
//	sem_t *wsem;
//	struct timeval tv;
};

#endif

