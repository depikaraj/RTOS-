/*
Server.c
It reads the input given by user in the format of first_number, second_number without spaces. To keep the simplicity of the arithmetic operation, user needs to give only single digit numbers for computing addition. After addition the server stores the result in result.txt file 
Server print son the terminal, the operand
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
 
struct my_msgbuf {
    long mtype;
    char mtext[200];
};

int main(void)
{
    struct my_msgbuf buf;
    int msqid;
    key_t key;
    int sum;
    
/*results.txt contains the computation solutions */
    FILE *fptr;
	
	fptr = fopen("results.txt", "a"); /* write in append mode */
	
	if(fptr == NULL)
	{
		printf("Error !");
		exit(1);
	}
   /* create a unique key using ftok */
    if ((key = ftok("client_mq.c", 'X')) == -1) {  /* same key as client_mq.c.c */
        perror("ftok");
        exit(1);
    }

    
  /* create a message queue */
    if ((msqid = msgget(key, 0644)) == -1) { /* connect to the queue */
        perror("msgget");
        exit(1);
    }
    
    printf("Server: ready to do computation.\n");   

    for(;;) 
      { 
		fptr = fopen("results.txt", "a");
        if (msgrcv(msqid, &buf, sizeof buf.mtext, 0, 0) == -1) 
        {
            perror("msgrcv");
            exit(1);
        }
        printf("Client  has sent: \"%s\"\n", buf.mtext);
	//printf("%c ", buf.mtext[0]);

	
	sum = buf.mtext[0]+ buf.mtext[1];

	/* Print results in file */
	fprintf(fptr, "%c belongs to Client:%c \n ", sum-'0', buf.mtext[3]);
	fclose(fptr);

	
    }
	
	



	

	

    return 0;
}
