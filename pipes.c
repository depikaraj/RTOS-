/* Assignment 1
C program that collect the output of one of these and pipe it as input into another – wihtout standard |  operator
Implement ls -1  *.jpg | wc –l  
*/ 

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(void)
{
	int pfds[2]; /* Pipe descriptors */
	
	pipe(pfds);

	/* Writing to pipe */
	if(!fork())
	{
		close(1); /*Close std out */ 
		dup(pfds[1]); /* duplicate */
		close(pfds[0]); /*dont allow reading while writing */
		execlp("ls", "ls","-l", NULL); /* Running $ls -1  *.jpg */
	}

	/* Reading from pipe */
	else
	{
		close(0); /*close std in . */
		dup(pfds[0]); /*duplicate */
		close(pfds[1]); /*Dont allow writing while reading */
		/* Search for image4.jpg */
		execlp("grep", "grep", "image4.jpg", NULL); /*Running $grep*/
	}
	return 0;
}
