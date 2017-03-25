/* IP phone server side 
Bandwidth = Data Rate/Sampling rate
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>/* IP phone with periodic scheduling server side */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
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
uint8_t buf[BUFSIZE];
volatile sig_atomic_t keep_going = 1;
int numbytes;
int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
static const pa_sample_spec ss = {
     					 .format = PA_SAMPLE_S16LE,
        				 .rate = 44100,
        				 .channels = 2
   					 };
pa_simple *s1 = NULL;
int ret = 1;
int error;
int fd;
#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define BACKLOG 10     // how many pending connections queue will hold

/*handler for CTRL+C SIGINT */
void my_handler_for_sigint(int signumber)
{
	char ans[2];
	if (signumber == SIGINT)
	{
		printf("received SIGINT\n");
		printf("Program received a CTRL-C\n");
		printf("Terminate Y/N : "); 
		scanf("%s", ans);
		if (strcmp(ans,"Y") == 0)
		{
			printf("Exiting ....Press any key\n");
			keep_going = 0;

		}
	else
	{
		printf("Continung ..\n");
	}
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
        	return &(((struct sockaddr_in*)sa)->sin_addr);
    	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void periodic(int signum)
{
	//printf("recie");
	if ((numbytes = recv(new_fd, buf, BUFSIZE, 0)) == -1)
	{
		perror("recv");
		exit(1);
	}
		  
	/* ... and play it */
	if (pa_simple_write(s1, buf, sizeof(buf), &error) < 0)
	{
		fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
		if (s1)
        		pa_simple_free(s1);
	}

}
int main(int argc,char* argv[])
{	
	int rv;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];	
	if (argc != 2)
	{
        	fprintf(stderr,"usage:port number\n");
        	exit(1);
   	}
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) 
	{
       		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        	return 1;
    	}	

    // loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
        	if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
		{
			perror("server: socket");
			continue;
       		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
           		perror("setsockopt");
            		exit(1);
       		}

        	if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
		{//bind a name to the socket;has no address assigned to it
           		close(sockfd);
           		perror("server: bind");
            		continue;
       		}

       		break;
	}	
	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL) 
	{
        	fprintf(stderr, "server: failed to bind\n");
        	exit(1);
    	}
	if (listen(sockfd, BACKLOG) == -1) 
	{//listen for connections
        	perror("listen");
        	exit(1);
   	}


	printf("server: waiting for connections...\n"); 
	if (!(s1 = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) 
	{
        	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}

	//uint8_t buf[BUFSIZE];
        //ssize_t r;

	while(1)
        {
		sin_size = sizeof their_addr;
        	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);//accept a connection
	        if (new_fd == -1)
		{
        		perror("accept");
	        }

		inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		printf("server: got connection from %s\n", s);
		
		//Periodic Scheduling
		
		struct itimerval timer;

	 	/* Install periodic_task  as the signal handler for SIGVTALRM. */
	 	memset (&sa, 0, sizeof (sa));
	 	sa.sa_handler = &periodic ;
	 	sigaction (SIGVTALRM, &sa, NULL);

	 	/* Configure the timer to expire after 250m sec... */
	 	timer.it_value.tv_sec = 0;
	 	timer.it_value.tv_usec = 1;

	 	/* ... and every 250m sec after that. */
	 	timer.it_interval.tv_sec = 0;
	 	timer.it_interval.tv_usec = 1;

	 	/* Start a virtual timer. It counts down whenever this process is    executing. */
	 	setitimer (ITIMER_VIRTUAL, &timer, NULL);
		while(1);

	}
	//close(new_fd); 
	
	if (pa_simple_drain(s1, &error) < 0)
	{
        	fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}
	ret = 0;
	finish:
		if (s1)
        	pa_simple_free(s1);

	return ret;
	return 0;
}

