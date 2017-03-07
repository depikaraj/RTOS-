/* IP phone server side */

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
#define BUFSIZE 10240

volatile sig_atomic_t keep_going = 1;

#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define BACKLOG 10     // how many pending connections queue will hold

void sigchld_handler(int s)
{
    /*waitpid() might overwrite errno, so we save and restore it: */
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

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

int main(int argc,char* argv[])
{	
	int rv;
	int numbytes;
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	static const pa_sample_spec ss = {
     					 .format = PA_SAMPLE_S16LE,
        				 .rate = 44100,
        				 .channels = 2
   					 };
	pa_simple *s1 = NULL;
	int ret = 1;
	int error;
	int fd;
	if (signal(SIGINT, my_handler_for_sigint) == SIG_ERR)//register signal handler for SIGINT
	printf("\ncan't catch SIGINT\n");
	
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

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) 
	{
        	perror("sigaction");
        	exit(1);
   	}

	printf("server: waiting for connections...\n"); 
	if (!(s1 = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) 
	{
        	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        	goto finish;
    	}

	uint8_t buf[BUFSIZE];
        ssize_t r;

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


	while(keep_going)//as long as SIGINT is not received
 	{ 
	  
		close(sockfd); 
		if ((numbytes = recv(new_fd, buf, BUFSIZE, 0)) == -1)
		{
			perror("recv");
			exit(1);
    		}
  

        /* ... and play it */
        	if (pa_simple_write(s1, buf, sizeof(buf), &error) < 0)
		{
			fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
			goto finish;
		}
		usleep(200);
	}


	
    
	}

	close(new_fd);  
	if(keep_going==0)
	{
		close(sockfd);
		printf("cleanup before exiting parent");
		exit(0);
	}

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

