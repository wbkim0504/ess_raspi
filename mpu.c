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
}

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

	static int mpu_header_size = 10;
	int mpu_body_size = 0;
	int mpu_packet_size = 0;

	long int start_time;
	long int time_diff;
	struct timespec gettime_now;

	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec;

	/* maximum allowable SPU */
	if (mpu_cfg.num_SPU > MAX_SPU)
	{
		mpu_cfg.num_SPU = MAX_SPU;
	}

	/* SPU data */

	/*
	if (mpu_cfg.emulator_enable)
	{
		for (int i=0; i<mpu_cfg.num_SPU; i++)
		{
			char spi_cmd = mpu_cfg.cmd_SPU + i;
			data_size[i] = spi_emul(spi_cmd, &spu_msg_buff[i][0], MAX_BUFF_SIZE, &mpu_cfg, &emul_cfg);
			
			mpu_body_size += data_size[i];

			if (data_size[i] > 0)
			{
				spu_status[i] = 1;
			}
		}
	}
	else
	{
	}
	*/

	uint8	spu_msg_buff[MAX_BUFF_SIZE];
	memset(spu_msg_buff, 0, MAX_BUFF_SIZE);

	//int data_size = spu_comm(&spu_msg_buff[0], MAX_BUFF_SIZE, &mpu_cfg);
	int data_size = 0;
	mpu_body_size += data_size;

	mpu_packet_size = mpu_header_size + mpu_body_size;

	/* MPU message packet */
	/*
	mpu_msg_buff[0] = (uint8) mpu_cfg.type_MPU + mpu_cfg.mpu_id;
	mpu_msg_buff[1] = (uint8) (mpu_body_size >> 24);
	mpu_msg_buff[2] = (uint8) (mpu_body_size >> 16);
	mpu_msg_buff[3] = (uint8) (mpu_body_size >> 8);
	mpu_msg_buff[4] = (uint8) (mpu_body_size);

	mpu_msg_buff[5] = (uint8) mpu_cfg.version;
	mpu_msg_buff[6] = 0x00;		// MSG_TYPE (TBD)
	mpu_msg_buff[7] = mpu_msg_count++;
	mpu_msg_buff[8] = 0x00;		// error_code;
	mpu_msg_buff[9] = (uint8) mpu_cfg.num_SPU;
	mpu_msg_buff[10] = (uint8) mpu_cfg.num_HIP;
	mpu_msg_buff[11] = 0x00;
	*/
	/*
	mpu_msg_buff[0] = (uint8) mpu_cfg.type_MPU + mpu_cfg.mpu_id;
	mpu_msg_buff[1] = (uint8) (mpu_body_size >> 8);
	mpu_msg_buff[2] = (uint8) (mpu_body_size);

	mpu_msg_buff[3] = (uint8) mpu_cfg.version;
	mpu_msg_buff[4] = (uint8) mpu_cfg.num_SPU;
	*/
	mpu_msg_buff[0] = 0x10;
	mpu_msg_buff[1] = (uint8) (mpu_packet_size >> 24);
	mpu_msg_buff[2] = (uint8) (mpu_packet_size >> 16);
	mpu_msg_buff[3] = (uint8) (mpu_packet_size >> 8);
	mpu_msg_buff[4] = (uint8) (mpu_packet_size);
	mpu_msg_buff[5] = (uint8) mpu_cfg.version;

	mpu_msg_buff[6] = mpu_cfg.mpu_id;
	mpu_msg_buff[7] = 0x00;		// error_code;
	mpu_msg_buff[8] = (uint8) mpu_cfg.num_SPU;
	//mpu_msg_buff[9] = (uint8) mpu_cfg.num_HIP;

	int pos = mpu_header_size;

	// SPU data
	/*
	for (int i=0; i<mpu_cfg.num_SPU; i++)
	{
		if (data_size[i] > 0)
		{
			memcpy(&mpu_msg_buff[pos], &spu_msg_buff[i][0], data_size[i]);
			pos += data_size[i];
		}
	}
	*/

	if (mpu_msg_count >= 100)
		mpu_msg_count = 0;

	/* send pms message */
	if (mpu_cfg.pms_connection == 0)
	{
		// connect server
		if (server_connect() > 0)
			mpu_cfg.pms_connection = 1;
		else
			mpu_cfg.pms_connection = 0;
	}

	if (mpu_cfg.pms_connection)
	{
		printf("sending %d byte data to server\n", pos);

		if (write(server_sockfd, mpu_msg_buff, pos) <= 0) {
			perror("write error : ");
			mpu_cfg.pms_connection = 0;
		}
	}

	/* elapsed time check */
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	time_diff = gettime_now.tv_nsec - start_time;
	if (time_diff < 0)
		time_diff += 1e9;

	//printf("communication time : %.3f msec \n", time_diff*(1e-6));
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
			data_comm();
			//data_comm_test();

			alarm_count = 0;

			if (tick_count >= 10)
			{
				tick_count = 0;
			}
		}
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
	mpu_cfg.spi_status = 0;

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
	printf("mpu_cfg.num_SPU = %d\n", mpu_cfg.num_SPU);

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

