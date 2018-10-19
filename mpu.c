#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mpu_config.h"
#include "mpu_conio.h"
#include "mpu_display.h"

#include "spi_comm.h"
//#include "spi_emul.h"

//#define	MAX_SPU		(10)

typedef unsigned char uint8;

// -----------------------------------------------------------------
// global variables
//
mpu_config_t	mpu_cfg;
//emul_config_t	emul_cfg;

uint8	mpu_msg_buff[(MAX_SPU+2)*MAX_BUFF_SIZE];

uint8	mpu_msg_count = 0;
int		server_sockfd = -1;
int		emul_id = 0;

pthread_t	tid = 0;

unsigned char	spi_count = 0;

// -----------------------------------------------------------------
void *server_msg(void *arg)
{
	int* pfd = (int*) arg;
	int	sockfd = (*pfd);
	int rlen;
	char buf[MAX_BUFF_SIZE];

	memset(buf, 0x00, MAX_BUFF_SIZE);
	while( (rlen = read(sockfd, buf, (MAX_BUFF_SIZE-1))) > 0) 
	{
		//buf[rlen] = '\0';
		//strip_newline(buf);
		printf("[server msg]:%s\n", buf);
	}
	/* server connection closed */
	close(server_sockfd);
	server_sockfd = -1;
	mpu_cfg.pms_connection = 0;
}

// -----------------------------------------------------------------
int server_connect()
{
	struct sockaddr_in serveraddr;
	int client_len;
	char buf[MAX_BUFF_SIZE];

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("error : ");
		return -1;
	}
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(mpu_cfg.pms_ip);
	serveraddr.sin_port = htons(mpu_cfg.pms_port);

	client_len = sizeof(serveraddr);

	if (connect(server_sockfd, (struct sockaddr*)&serveraddr, client_len) == -1) 
	{
		//perror("connect error : ");
		close(server_sockfd);
		server_sockfd = -1;
		return -1;
	}

	/* thread for getting server message */
	if (tid != 0)
	{
		// kill & wait previous thread
	}
	pthread_create(&tid, NULL, &server_msg, (void*) &server_sockfd);

	return server_sockfd;
}


// -----------------------------------------------------------------
void data_comm()
{
	long int start_time;
	long int time_diff;
	struct timespec gettime_now;

	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec;


	/* mpu packet header */
	unsigned char packet_type = 0x10;
	unsigned char mpu_status = 0x00;

	mpu_msg_buff[0] = packet_type;
	/*
	mpu_msg_buff[1] = (uint8) (mpu_packet_size >> 24);
	mpu_msg_buff[2] = (uint8) (mpu_packet_size >> 16);
	mpu_msg_buff[3] = (uint8) (mpu_packet_size >> 8);
	mpu_msg_buff[4] = (uint8) (mpu_packet_size);
	*/
	mpu_msg_buff[5] = 0x00;		// reserved

	mpu_msg_buff[6] = mpu_cfg.mpu_id;
	mpu_msg_buff[7] = mpu_status;
	mpu_msg_buff[8] = (uint8) mpu_cfg.num_SPU;
	mpu_msg_buff[9] = (uint8) mpu_cfg.num_HIP;

	int mpu_header_size = 10;
	int msg_curr_pos = mpu_header_size;

	/* mpu packet body */
	int mpu_body_size = 0;
	int mpu_packet_size = 0;

	/* SPU data */
	int spu_header_size = 9;
	int spu_packet_size = 0;

	uint8	spu_msg_buff[MAX_BUFF_SIZE];
	memset(spu_msg_buff, 0, MAX_BUFF_SIZE);

	int spu_count = 0;
	for (int i=0; i<MAX_SPU; i++)
	{
		if (mpu_cfg.SPU_info[i])
		{
			spu_count++;

			int spu_status = 0;		// 0:normal, 1:error
			int spu_body_size = 0;

			int curr_dev_id = SPI_DEV_SPU1 + i;
			int data_size = micom_get_data (curr_dev_id, SPI_ECU_NONE, SPI_CMD_DATA, 
										spu_msg_buff, MAX_BUFF_SIZE, &mpu_cfg);
		
			if (mpu_cfg.CRC_status)
			{
				spu_body_size = data_size;
			}
			else
			{
				/* CRC error */
				spu_status = 1;
				spu_body_size = 0;
			}

			/* spu header */
			unsigned char spu_header[9];
			spu_header[0] = i+1;
			spu_header[1] = (uint8) (spu_body_size >> 8);
			spu_header[2] = (uint8) (spu_body_size);
			spu_header[3] = spu_status;

			spu_header[4] = mpu_cfg.ECU_info[i][0];
			spu_header[5] = mpu_cfg.ECU_info[i][1];
			spu_header[6] = mpu_cfg.ECU_info[i][2] + mpu_cfg.ECU_info[i][3];
			spu_header[7] = mpu_cfg.ECU_info[i][4];
			spu_header[8] = mpu_cfg.ECU_info[i][5];
			
			spu_packet_size = spu_header_size + spu_body_size;
			
			memcpy(&mpu_msg_buff[msg_curr_pos], spu_header, spu_header_size);
			msg_curr_pos += spu_header_size;

			if (spu_body_size > 0)
			{
				memcpy(&mpu_msg_buff[msg_curr_pos], spu_msg_buff, spu_body_size);
				msg_curr_pos += spu_body_size;
			}
		}

	}

	/* HIP data */
	int hip_count = 0;
	for (int i=0; i<MAX_HIP; i++)
	{
		if (mpu_cfg.HIP_info[i])
		{
			hip_count++;

			int hip_status = 0;		// 0:normal, 1:error
			int hip_body_size = 0;

			int curr_dev_id = SPI_DEV_HIP1 + i;
			int data_size = micom_get_data (curr_dev_id, SPI_ECU_NONE, SPI_CMD_DATA, 
										spu_msg_buff, MAX_BUFF_SIZE, &mpu_cfg);
		
			if (mpu_cfg.CRC_status)
			{
				if (data_size > 0)
				{
					memcpy(&mpu_msg_buff[msg_curr_pos], spu_msg_buff, data_size);
					msg_curr_pos += data_size;
				}
			}
			else
			{
				/* CRC error */
			}

		}
	}
	mpu_packet_size = msg_curr_pos;

	mpu_msg_buff[1] = (uint8) (mpu_packet_size >> 24);
	mpu_msg_buff[2] = (uint8) (mpu_packet_size >> 16);
	mpu_msg_buff[3] = (uint8) (mpu_packet_size >> 8);
	mpu_msg_buff[4] = (uint8) (mpu_packet_size);


	/* send pms message */
	if (mpu_cfg.pms_connection)
	{
		printf("\n");
		printf("sending %d byte data to server\n", mpu_packet_size);

		if (write(server_sockfd, mpu_msg_buff, mpu_packet_size) <= 0) {
			perror("write error : ");
			mpu_cfg.pms_connection = 0;
		}
	}

	/* elapsed time check */
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	time_diff = gettime_now.tv_nsec - start_time;
	if (time_diff < 0)
		time_diff += 1e9;

	printf("\n");
	printf("communication time : %.3f msec \n", time_diff*(1e-6));
	printf("server connection : %s\n", mpu_cfg.pms_connection ? "OK" : "NO");

	/* server connect */
	if (mpu_cfg.pms_connection == 0)
	{
		printf("connecting to pms server ... \n");

		// connect server
		if (server_connect() > 0)
			mpu_cfg.pms_connection = 1;
		else
			mpu_cfg.pms_connection = 0;
	}
}

// -----------------------------------------------------------------
//int alarm_count = 0;
int tick_count = 0;

void fn_alarm(int sig_num)
{
	if (sig_num == SIGALRM)
	{
		//printf("fn_alarm called : %d \n", tick_count++);
		if (tick_count++ == 0)
		{
			if (get_curr_menu() == MPU_MENU_RUN)
			{
				data_comm();
				mpu_display_update(&mpu_cfg);
			}		
		}
		tick_count %= 10;
	}
}


// -----------------------------------------------------------------
int main(int argc, char *argv[])
{
	/* get config */
	if (get_config(&mpu_cfg) != EXIT_SUCCESS)
	{
		printf("error in reading config file \n");
		return (EXIT_FAILURE);
	}
	mpu_cfg.pms_connection = 0;
	mpu_cfg.CRC_status = 0;

	//set_config(&mpu_cfg);
	
	/* test big-endian */
	/*
	printf("0x%08X \n", 0x01020304);
	printf("%d \n", (char) 0x01020304); //--> 4
	printf("%d \n", (char) (0x01020304 >> 8)); //--> 
	printf("%d \n", (char) (0x01020304 >> 16)); //--> 
	printf("%d \n", (char) (0x01020304 >> 24)); //--> 
	*/

	/* print mpu running info */
	//printf("mpu_cfg.num_SPU = %d\n", mpu_cfg.num_SPU);

	/* init console & alarm */
	set_conio_terminal_mode();
	signal(SIGALRM, fn_alarm);
	ualarm(100000,100000);		// 100 msec timer
	//ualarm(10000,10000);		// 10 msec timer

	mpu_display(MPU_MENU_MAIN, &mpu_cfg);

	/* main_loop */
    while (1)
	{
		if (kbhit()) 
		{
			int ch = getch();
			if (ch == 'q')
			{
				printf("--> %c key pressed : program end !!\n\n", ch);
				break;
			}
			else
			{
				//printf("--> %c (0x%04X) key pressed \n", ch, ch);
				mpu_key_handle(ch, &mpu_cfg);
			}
		}
        /* do some work */
    }

	return (EXIT_SUCCESS);
}

