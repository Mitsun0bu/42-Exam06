#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define	ARGS	"Wrong number of arguments\n"
#define BUFF	200000
#define	FATAL	"Fatal error\n"
#define PORT	65000

int		maxfd, sockfd, id = 0;
int		arr[PORT];
char	buff[BUFF + 100];
fd_set	rset, wset, aset;

void	error(char *msg)
{
	if (sockfd > 2)
		close(sockfd);
	write(2, msg, strlen(msg));
	exit(1);
}

void	replyAll(int connfd)
{
	for (int fd = 2; fd <= maxfd; fd++)
		if (fd != connfd && FD_ISSET(fd, &wset))
			if (send(fd, buff, strlen(buff), 0) < 0)
				error(FATAL);
}

int main(int argc, char **argv)
{
	int					connfd;
	socklen_t			len;
	struct sockaddr_in	servaddr, cli; 

	if (argc != 2)
		error(ARGS);

	// CREATE SOCKET AND CHECK FOR ERRORS 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		error(FATAL);
	bzero(&servaddr, sizeof(servaddr)); 

	// ASSIGN IP & PORT 
	servaddr.sin_family		 = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port		 = htons(atoi(argv[1])); 
  
	// BIND NEW SOCKET TO IP
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error(FATAL);

	// LISTEN FOR CONNECTIONS
	if (listen(sockfd, BUFF) != 0)
		error(FATAL);

	len		= sizeof(cli);
	maxfd	= sockfd;
	FD_ZERO(&aset);
	FD_SET(sockfd, &aset);

	while (1)
	{
		// SELECT 
		rset = wset = aset;
		if (select(maxfd + 1, &rset, &wset, 0, 0) < 0)
			continue;

		// ACCEPT NEW CONNECTION AND ADD IT TO FD_SET
		if (FD_ISSET(sockfd, &rset))
		{
			connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
			if (connfd < 0)
				error(FATAL);

			sprintf(buff, "server: client %d just arrived\n", id);

			arr[connfd] = id++;
			FD_SET(connfd, &aset);

			replyAll(connfd);
			maxfd = connfd > maxfd ? connfd : maxfd;
	
			continue;
		}

		// READ FROM CLIENT
		for (int fd = 2; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &rset))
			{
				int		r = 1;
				char	msg[BUFF];
				bzero(&msg, sizeof(msg));
		
				while (r == 1 && msg[strlen(msg) - 1] != '\n')
					r = recv(fd, msg + strlen(msg), 1, 0);
				if (r <= 0)
				{
					sprintf(buff, "server: client %d just left\n", arr[fd]);
					FD_CLR(fd, &aset);
					close(fd);
				}
				else
					sprintf(buff, "client %d: %s", arr[fd], msg);

				replyAll(fd);
			}
		}
	}
	return (0);
}