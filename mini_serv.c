#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct client {
	int id;
	int fd;
} t_client;

t_client clients[9999];
char buffer[1000000];
char msg[1000000];
int	max_fd;
int id = 0;
int min_available_index = 0;
fd_set readfds, writefds, active;

void	fatal()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	add_client(int fd)
{
	clients[min_available_index].fd = fd;
	clients[min_available_index].id = id;
	id++;
	min_available_index++;
}

void	send_all(char *to_send, int sender)
{
	for (int i = 0; i <= 9999; i++)
	{
		if (clients[i].fd == -1 || clients[i].fd == sender)
			continue;
		if (FD_ISSET(clients[i].fd, &writefds))
			send(clients[i].fd, to_send, strlen(to_send), 0);
	}
}

int main(int ac, char **av) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;
	int ret_select;
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	int port = atoi(av[1]);
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		fatal();
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal();
	if (listen(sockfd, 128) != 0)
		fatal();
	len = sizeof(cli);
	FD_ZERO(&readfds);
	FD_ZERO(&active);
	FD_SET(sockfd, &active);
	max_fd = sockfd;
	while(1)
	{
		readfds = writefds = active;
		ret_select = select(max_fd + 1, &readfds, &writefds, NULL, NULL);
		if (ret_select > 0)
		{
			if (FD_ISSET(sockfd, &readfds))
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
				if (connfd < 0)
					fatal();
				add_client(connfd);
				FD_SET(connfd, &active);
				char to_send[1024];
				sprintf(to_send, "server: client %d just arrived\n", id - 1);
				send_all(to_send, connfd);
				if (max_fd < connfd)
					max_fd = connfd;
			}
			else
			{
				for (int i = 0; i <= id; i++)
				{
					if (clients[i].fd == -1)
						continue;
					if (FD_ISSET(clients[i].fd, &readfds))
					{
						int ret_recv;
						ret_recv = recv(clients[i].fd, buffer, 1000000, 0);
						if (ret_recv <= 0)
						{
							char to_send[1024];
							close(clients[i].fd);
							FD_CLR(clients[i].fd, &active);
							FD_CLR(clients[i].fd, &readfds);
							sprintf(to_send, "server: client %d just left\n", clients[i].id);
							send_all(to_send, clients[i].fd);
							clients[i].fd = -1;
							clients[i].id = -1;
							if (i < min_available_index)
								min_available_index = i;
							bzero(buffer, sizeof(buffer));
							bzero(msg, sizeof(msg));
							continue;
						}
						int j = 0;
						int beg = 0;
						while (j < ret_recv)
						{
							if (buffer[j] == '\n' || buffer[j] == '\0')
							{
								char to_send[1050000];
								buffer[j] = 0;
								sprintf(to_send, "client %d: %s\n", clients[i].id, buffer + beg);
								printf("Sending to all : %s\n", to_send);
								send_all(to_send, clients[i].fd);
								beg = j + 1;
								j++;
							}
							else
								j++;
						}
						bzero(buffer, sizeof(buffer));
						bzero(msg, sizeof(msg));
					}
				}
			}
		}
	}

}
