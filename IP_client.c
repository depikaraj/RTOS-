//IP phone client side

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include<signal.h>
#include <arpa/inet.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#define BUFSIZE 1024

volatile sig_atomic_t keep_going = 1;

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}	

		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*Signal handler for ctrl+C */
void my_handler_for_sigint(int signumber)
{
	char ans[2];
	if (signumber == SIGINT)
	{
		printf("received SIGINT\n");
		printf("Program received a CTRL-C\n");
		printf("Terminate Y/N : "); 
		scanf("%s", ans);
		if (strcmp(ans,"Y") == 0)/* Exit if Y */
		{		
			printf("Exiting ....Press any key\n");
			keep_going = 0;
			exit(0); 
		}
	
	else
	{
		printf("Continung ..\n");
	}
  }

}


int main(int argc, char *argv[])
{
	int sockfd,l;  
	char str[80];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int ch;
	char s[INET6_ADDRSTRLEN];
	static const pa_sample_spec ss = {
        				.format = PA_SAMPLE_S16LE,
        				.rate = 44100,
        				.channels = 2
   					 };
	pa_simple *s1 = NULL;
	pa_simple *s2=NULL;
	int ret = 1;
	int error;
	int fd;

    	/* Create the recording stream */
	if (!(s1 = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error)))
	{
        	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}
	printf("creating recording stream\n");
        uint8_t buf[BUFSIZE];


	if (argc != 3)
	{	/*check for format of arguments*/
        	fprintf(stderr,"usage: client hostname\n");
        	exit(1);
   	}
	if (signal(SIGINT, my_handler_for_sigint) == SIG_ERR)//register signal handler
	printf("\ncan't catch SIGINT\n");

	memset(&hints, 0, sizeof hints);//fills the struct with 0
	hints.ai_family = AF_UNSPEC;//v4 or v6
	hints.ai_socktype = SOCK_STREAM;//stream as opposed to datagram

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
	{//given a host and a service returns a struct of address info
        	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        	return 1;
    	}


    	/*loop through all the results and connect to the first we can */
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
       		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
		{
           		 perror("client: socket");
           		 continue;
       		}

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
	{
		close(sockfd);
           	perror("client: connect");
            	continue;
        }

        break;
    }

	if (p == NULL) 
	{
        	fprintf(stderr, "client: failed to connect\n");
        	return 2;
    	}	

	ssize_t r;
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);//convert ipv4 and v6 addresses from binary to text,af,src,dst,size
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure
	while(keep_going)//loop for the child which keeps sending messages
	{
        	/* Record some data ... */
		if (pa_simple_read(s1, buf, sizeof(buf), &error) < 0)
		{
            		fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            		goto finish;
        	}
		
		if (send(sockfd, buf, sizeof(buf), 0) == -1)
		{ 
			perror("send");
			continue;
		    	close(sockfd);
		    	exit(0);
		}
	usleep(200);
	}

	/*Cleanup after signal handling */
	if (send(sockfd,"exit",4, 0) == -1)//send keyword called "EXIT" so that server can stop listening to this client
	{ 
		perror("send");
		//continue;
            	close(sockfd);
            	exit(0);
	}
	close(sockfd);
	exit(0);
	return 0;  
	ret = 0;
	finish:
		if (s1)
		pa_simple_free(s1);
	return ret;
}
