/*
Implement Semaphore for the preventing accidental over write while reading in progress
Implementation of semaphores using shared memory as critical section which can be used by parent or child process one at a time.
If parent is writing child acnt access the semaphore 
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include<string.h>
#include <sys/shm.h>
#include<sys/wait.h>
union semun {
	int val; //for SETVAL of specifed semaphore
	struct semid_ds *buf; //for IPC_STAT and IPC_SET
	ushort *array;  //unsigned short integer array pointer for GETALL and SETALL
	};

int main()
{	char ch;
	int key,key2,share_id,num;
	char *data;
	int semid;
	union semun arg;
	arg.val = 1;
	struct sembuf sb={0,-1,0};

	// Shared memory creation 
	key=ftok(".",'b'); //generate a random key
	
	if(key == -1 ) 
	{
		printf("\n\n Initialization Falied of shared memory \n\n");
		return 1;
	}
	
	share_id=shmget(key,1024,IPC_CREAT|0744);
	printf("Shared memory id = %d \n", share_id);
	if(share_id == -1 ) 
	{
		printf("\n\n Error captured while share memory allocation\n\n");
		return 1;
	}
	data=(char *)shmat(share_id,(void *)0,0);
	strcpy(data,"Semaphore implementation \n");

	
	//Semaphore creation
	key2 = ftok(".",'Z');
	semid = semget(key2, 1, 0666 | IPC_CREAT);
	printf("Semaphore id %d \n", semid);

	//Parent locks first
	sb.sem_op=-1; //Lock
	sb.sem_flg = SEM_UNDO|IPC_NOWAIT;
	if((semop(semid,(struct sembuf *)&sb,1)) == -1)
	{
		perror("semop");
	}
	printf("Parent got access \n");
	strncat(data,"Parent writing with the help of semaphore \n",44);
	printf ("%s\n",data);
	printf("If child tries to acces the semaphore \n");
	/*Child will got get access and semop will throw an error */
	if(!fork())
	{ //Child Porcess
		while(1)
		{
		if((semop(semid,(struct sembuf *)&sb,1)) == -1)
		{
			perror("semop"); //semop error is thrown
			exit(0);
		}
		exit(0);
		}
	} 
	
	return 0;
}
