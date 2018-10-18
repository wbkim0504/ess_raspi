#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "mpu_conio.h"

#define SERVER_PORT		(5000)
#define MAXLINE 1024

//char server_addr[] = "147.46.119.144";
char server_addr[] = "127.0.0.1";
int	server_sockfd = -1;

// -----------------------------------------------------------------
void strip_newline(char *s)
{
	while(*s != '\0')
	{
		if (*s == '\r' || *s == '\n')
			*s = '\0';
		s++;
	}
}

// -----------------------------------------------------------------
void print_buff(unsigned char *buff, int buff_len)
{
	for (int i=0; i<buff_len; i++)
	{
		printf("0x%02X ", buff[i]);
		if ( (i+1)%10 == 0)
		{
			printf("\n");
		}
	}
	printf("\n");
}


// -----------------------------------------------------------------
void *handle_recv(void *arg)
{
	int* pfd = (int*) arg;
	int	sockfd = (*pfd);
	int rlen;
	unsigned char buf[MAXLINE];

	memset(buf, 0x00, MAXLINE);
	while( (rlen = read(sockfd, buf, (MAXLINE-1))) > 0) 
	{
		/*
		buf[rlen] = '\0';
		strip_newline(buf);
		printf("[server]:%s\n", buf);
		*/
		printf(">> %d bytes received \n", rlen);
		print_buff(buf, rlen);
	}
	printf("handle_recv : end \n");
	server_sockfd = -1;
}


// -----------------------------------------------------------------
int server_connect()
{
	struct sockaddr_in serveraddr;
	int client_len;

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("error : ");
		return -1;
	}
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(server_addr);
	serveraddr.sin_port = htons(SERVER_PORT);

	client_len = sizeof(serveraddr);

	if (connect(server_sockfd, (struct sockaddr*)&serveraddr, client_len) == -1) {
		perror("connect error : ");
		//fprintf(stderr, "connect error : \n");
		server_sockfd = -1;
		return -1;
	}

	pthread_t	tid;
	pthread_create(&tid, NULL, &handle_recv, (void*) &server_sockfd);

	return server_sockfd;
}


// -----------------------------------------------------------------
int get_word_data(unsigned char *buff)
{
	int res = (buff[0] << 8) + buff[1];
	return res;
}


// -----------------------------------------------------------------
void set_word_data(unsigned char *buff, int data)
{
	buff[0] = (unsigned char) ( (data >> 8) & 0xFF );
	buff[1] = (unsigned char) ( data & 0xFF );
}


// -----------------------------------------------------------------
int send_request()
{
	unsigned char buff[1024];

	int packet_len = 12;

	int transaction_id	= 1;			// increased by 1
	int protocol_id		= 0x0000;		// Fixed
	int data_len		= 6;
	int unit_id			= 1;
	int function_code	= 0x04;			// Fixed
	int ref_number		= 0x0000;		// Resister
	int word_count		= 1;			// number of data

	set_word_data(&buff[0], transaction_id);
	set_word_data(&buff[2], protocol_id);
	set_word_data(&buff[4], data_len);
	buff[6] = (unsigned char) unit_id;
	buff[7] = (unsigned char) function_code;
	set_word_data(&buff[8], ref_number);
	set_word_data(&buff[10], word_count);

	if (server_sockfd != -1)
	{
		printf(">> sending %d bytes \n", packet_len);
		print_buff(buff, packet_len);

		if (write(server_sockfd, buff, packet_len) <= 0) 
		{
			perror("write error : ");
			return 0;
		}
	}
}


// -----------------------------------------------------------------
int alarm_count = 0;
int tick_count = 0;

void fn_alarm(int sig_num)
{
	if (sig_num == SIGALRM)
	{
		//data_comm_test();
		if (alarm_count++ >= 10)
		{
			//printf("alarm : count = %02d \r\n", tick_count++);
			alarm_count = 0;

			if (tick_count >= 10)
			{
				tick_count = 0;
			}
		}
	}
}


// ----------------------------------------------------------------------------
int main(int argc, char **argv) 
{
	char buf[MAXLINE];

	/* server connection */
	if (server_connect() == -1)
	{
		printf("server connection error !\n");
		return 0;
	}
	else
	{
		printf("server connected \n");
	}

	/* set timer */
	set_conio_terminal_mode();
	signal(SIGALRM, fn_alarm);
	ualarm(100000,100000);		// 100 msec timer

	while(1)
	{
		if (kbhit()) 
		{
			int ch = getch();
			if (ch == 'q')
			{
				printf("--> %c key pressed : program end !!\n\n", ch);
				break;
			}
			else if (ch == ' ')
			{
				send_request();
			}
			else
			{
				//printf("--> %c (0x%04X) key pressed \n", ch, ch);
				//mpu_key_handle(ch, &mpu_cfg);
			}
		}
	}
	close(server_sockfd);
	return 0;
}




