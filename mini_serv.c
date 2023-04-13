#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// DEFINE CONSTANTS
#define	ARGS	"Wrong number of arguments\n"
#define BUFF	200000
#define	FATAL	"Fatal error\n"
#define PORTS	65000

// DECLARE GLOBAL VARIABLES
int		maxfd, sockfd, id = 0;
int		client_id_arr[PORTS];
char	msg_buff[BUFF + 100];
fd_set	rset, wset, aset;

// FUNCTION TO HANDLE ERRORS
void	error(char *msg)
{
	if (sockfd > 2)
		close(sockfd);
	write(2, msg, strlen(msg));
	exit(1);
}

// FUNCTION TO SEND A MESSAGE TO ALL CONNECTED CLIENT, EXCEPT THE SENDER
void	replyToAll(int connfd)
{
	for (int fd = 2; fd <= maxfd; fd++)
		if (fd != connfd && FD_ISSET(fd, &wset))
			if (send(fd, msg_buff, strlen(msg_buff), 0) < 0)
				error(FATAL);
}

int main(int ac, char **av)
{
	int					connfd;
	socklen_t			len;
	struct sockaddr_in	servaddr, cliaddr; 

	// CHECK THE NUMBER OF ARGUMENTS
	if (ac != 2)
		error(ARGS);

	// CREATE SOCKET AND CHECK FOR ERRORS 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		error(FATAL);

	// SET SERVER'S IP ADDRESS & PORT NUMBER 
	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family		 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port		 = htons(atoi(av[1]));

	// BIND THE SERVER SOCKET TO IP ADDRESS AND PORT
	if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
		error(FATAL);

	// LISTEN FOR INCOMING CLIENT CONNECTIONS
	if (listen(sockfd, 4096) != 0)
		error(FATAL);

	// START THE SERVER LOOP
	len		= sizeof(cliaddr);
	maxfd	= sockfd;
	FD_ZERO(&aset);
	FD_SET(sockfd, &aset);

	while (1)
	{
		// SET UP THE FD SETS FOR THE select() FUNCTION
		rset = wset = aset;
		if (select(maxfd + 1, &rset, &wset, 0, 0) < 0)
			continue;

		// CHECK IF THERE IS A NEW INCOMING CLIENT CONNECTION
		if (FD_ISSET(sockfd, &rset))
		{
			// ACCEPT NEW CONNECTION AND ADD IT TO FD_SET
			connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
			if (connfd < 0)
				error(FATAL);

			// UPDATE THE MESSAGE BUFFER TO NOTIFY ABOUT NEW CLIENT 
			sprintf(msg_buff, "server: client %d just arrived\n", id);

			// UPDATE THE CLIENT ID ARRAY
			client_id_arr[connfd] = id++;
			FD_SET(connfd, &aset);

			// SEND THE ARRIVAL MESSAGE TO ALL CONNECTED CLIENTS
			replyToAll(connfd);
			maxfd = (connfd > maxfd) ? connfd : maxfd;
			continue;
		}

		// ITERATE THROUGH ALL CONNECTED CLIENTS TO CHECK FOR INCOMING MESSAGES
		for (int fd = 2; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &rset))
			{
				int		recv_return = 1;
				char	msg[BUFF];
				bzero(&msg, sizeof(msg));

				// READ A MESSAGE FROM THE CLIENT UNTIL A NEW LINE IS FOUND
				while (recv_return == 1 && msg[strlen(msg) - 1] != '\n')
					recv_return = recv(fd, msg + strlen(msg), 1, 0);
				if (recv_return <= 0)
				{
					// UPDATE THE MESSAGE IF THE CLIENT DISCONNECTS OR AN ERROR OCCURS
					sprintf(msg_buff, "server: client %d just left\n", client_id_arr[fd]);
					FD_CLR(fd, &aset);
					close(fd);
				}
				else
					// UPDATE THE MESSAGE IF A MESSAGE IS RECEIVED
					sprintf(msg_buff, "client %d: %s", client_id_arr[fd], msg);

				// SEND THE DEPARTURE MESSAGE OR REGULAR MESSAGE TO ALL OTHER CONNECTED CLIENTS
				replyToAll(fd);
			}
		}
	}
	return (0);
}