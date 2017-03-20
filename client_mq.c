/*
Client.c
Client sends two numbers to server without giving any sapce in the string.This string is passed to server by a messgae queue.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
struct my_msgbuf {
    long mtype;
    char mtext[200];
};

int main(void)
{
    srand( time(NULL));
    struct my_msgbuf buf;
    int msqid;
    key_t key;
    char ch;
  /* create a unique key using ftok */
    if ((key = ftok("client_mq.c", 'X')) == -1) {
        perror("ftok");
        exit(1);
    }
  /* create a message queue */
    if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    } 
    printf("Enter numbers to send to server, ^D to quit:\n");
    buf.mtype = 1; /* we don't really care in this case */
    while(fgets(buf.mtext, sizeof buf.mtext, stdin) != NULL) 
    {
        int len = strlen(buf.mtext);	
	buf.mtext[len-1] =rand()%9 + '0'; /* Generate a random number and send it as client id by appending it to the end of string */
	len ++;

        /* ditch newline at end, if it exists */
        if (buf.mtext[len-1] == '\n') buf.mtext[len-1] = '\0';
	//printf("%s", buf.mtext);
        if (msgsnd(msqid, &buf, len+1, 0) == -1) /* +1 for '\0' */
            perror("msgsnd");
    }

    if (msgctl(msqid, IPC_RMID, NULL) == -1) 
    {
        perror("msgctl");
        exit(1);
    }

    return 0;
}
