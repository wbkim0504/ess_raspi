#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>  
#include <linux/types.h>
#include <linux/spi/spidev.h>  

#include <bcm2835.h>
#include "spi_comm.h"

#define PIN_11 RPI_GPIO_P1_11

 
/*
static const char *device = "/dev/spidev0.0";  
static __u8 mode;  
static __u8 bits = 8;  
static __u32 speed = 250000;  
static __u16 delay;  
*/
  
int spi_fd = -1;  


// -----------------------------------------------------------------------
unsigned char crc_check(unsigned char *buff, int len)
{
	unsigned crc = 0;

	for (int i=0; i<len; i++)
	{
		crc ^= buff[i];

		for (int k=0; k<8; k++)
		{
			if (crc & 0x01)
				crc = (crc >> 1) ^ 0xB2;
			else
				crc = crc >> 1;
		}
	}	
	return crc;
}

// -----------------------------------------------------------------------
int spi_open()
{
	if (!bcm2835_init())
	{
		printf("bcm2835_init failed. Are you running as root??\n");
		return -1;
	}
	bcm2835_gpio_fsel(PIN_11, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(PIN_11, HIGH);

	if (!bcm2835_spi_begin())
	{
		printf("bcm2835_spi_begin failed. Are you running as root??\n");
		return -1;
	}
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // 

	bcm2835_spi_setClockDivider(400);	// user defined

	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_512);	// 781.25 kHz
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);	// 1.5625 MHz
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);	// 3.125 MHz
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);	// 6.25 MHz
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);	// 12.5 MHz
	//bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);	// 25 MHz
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
	return 1;
}

void spi_close()
{
	spi_fd = -1;
	bcm2835_close();
}


int spi_comm(unsigned char *tx, unsigned char *rx, int buff_size)
{
	unsigned int	msg_size = 0;

	if (spi_fd < 0)
		spi_fd = spi_open();

	if (spi_fd >= 0)
	{
		//printf("spi_comm() bcm2835_spi_begin() \n");

		bcm2835_gpio_write(PIN_11, LOW);
		bcm2835_spi_begin();

		//bcm2835_spi_transfernb(tx, rx, MAX_BUFF_SIZE);
		bcm2835_spi_transfernb(tx, rx, buff_size);

		bcm2835_spi_end();
		bcm2835_gpio_write(PIN_11, HIGH);
	}
	return buff_size;
}


#define DEV_NULL	(0x7D)

#define ERR_CRC		(0x01)
#define ERR_CMD		(0x02)

void send_error_code(unsigned char dev, unsigned char error_code)
{
	unsigned char tx[10] = {0x00,};
	unsigned char rx[10] = {0x00,};

	int p = 0;

	tx[p++] = dev;
	tx[p++] = 0x82;
	tx[p++] = 0x00;
	tx[p++] = 0x01;

	p += 4;

	tx[p++] = error_code;
	tx[p] = crc_check(tx, p);

	spi_comm(tx, rx, p+1);
}


/*
int micom_get_data (int dev, int ecu, int cmd,
					unsigned char *msg_buff, int buff_size, mpu_config_t *cfg)
{
	unsigned char dev_type[] = { 0x7E, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
	unsigned char ecu_type[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
	unsigned char cmd_type[] = { 0x00, 0x01, 0x02, 0x03 };

	// check input parameter
	if (dev > sizeof(dev_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in dev input (dev = %d)\n", dev);
		dev = 0;
	}

	if (ecu > sizeof(ecu_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in ecu input (ecu = %d)\n", ecu);
		ecu = 0;
	}

	if (dev > sizeof(dev_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in cmd input (cmd = %d)\n", cmd);
		dev = 0;
	}

	int data_size = 0;

	switch (ecu)
	{
		case SPI_ECU_INV :
			data_size = cfg->ecu_data_size[ECU_TYPE_INV] + 2;
			break;
		case SPI_ECU_PCS :
			data_size = cfg->ecu_data_size[ECU_TYPE_PCS] + 2;
			break;
		case SPI_ECU_LIP1 :
		case SPI_ECU_LIP2 :
			data_size = cfg->ecu_data_size[ECU_TYPE_LIP] + 2;
			break;
		case SPI_ECU_ESS :
			data_size = cfg->ecu_data_size[ECU_TYPE_ESS] + 2;
			break;
		case SPI_ECU_PRU :
			data_size = cfg->ecu_data_size[ECU_TYPE_PRU] + 2;
			break;
		case SPI_ECU_NONE :
			if (dev == SPI_DEV_MICOM)
			{
				data_size = 0;
				for (int i=0; i<MAX_SPU; i++)
				{
					if (cfg->SPU_info[i])
					{
						data_size += cfg->spu_data_size[i];
					}
				}
				data_size += cfg->num_HIP * (cfg->ecu_data_size[ECU_TYPE_HIP] + 2);
			}
			else if (dev >= SPI_DEV_SPU1 && dev <= SPI_DEV_SPU4)
			{
				int spu_id = dev - SPI_DEV_SPU1;
				data_size = cfg->spu_data_size[spu_id];
			}
			else if (dev >= SPI_DEV_HIP1 && dev <= SPI_DEV_HIP2)
			{
				data_size = cfg->ecu_data_size[ECU_TYPE_HIP] + 2;
			}
			break;
	}


	// check buff_size 
	if (buff_size < data_size)
	{
		fprintf(stderr, "buff_size (%d) < data_size (%d) in micom_get_data()\n", 
				buff_size, data_size);
		return 0;
	}


	unsigned char dev_list[] = { DEV_NULL, 0x00, DEV_NULL};
	unsigned char cmd_list[] = { 0x00, 0x00, 0x00 };
	unsigned int size_list[] = { 0, 0, 0 };

	dev_list[1] = dev_type[dev];
	cmd_list[1] = ecu_type[ecu] | cmd_type[cmd];
	size_list[1] = data_size;
 

	// spi comm 
	unsigned char tx[MAX_BUFF_SIZE] = {0,};
	unsigned char rx[MAX_BUFF_SIZE] = {0,};
	unsigned char crc;

	int recv_data_size = 0;

	for (int i=0; i<2; i++)
	{
		// tx[] = devicei(1) cmd(1), data_size(2), next_cmd(4), crc(1)
		int p = 0;

		tx[p++] = dev_list[i];
		tx[p++] = cmd_list[i];
		tx[p++] = (size_list[i] >> 8) & 0xFF;
		tx[p++] = size_list[i] & 0xFF;

		tx[p++] = dev_list[i+1];
		tx[p++] = cmd_list[i+1];
		tx[p++] = (size_list[i+1] >> 8) & 0xFF;
		tx[p++] = size_list[i+1] & 0xFF;

		p += size_list[i];
		tx[p] = crc_check(tx, p);
		
		// tx[]
		printf("--> tx[0-8] : %d bytes \n", p+1);
		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", tx[k]);
		}
		printf(": CRC(0x%02X)\n", tx[p]);

		spi_comm(tx, rx, p+1);
		//printf(" spi_comm() called !\n");

		recv_data_size = (rx[2] << 8) + rx[3];
		if (recv_data_size <= (p-8))
		{
			p = recv_data_size + 8;
		}
		else
		{
			// error case 
			recv_data_size = p - 8;
		}
		//printf(" p = %d \n", p);
		unsigned char crc = crc_check(rx, p);

		// rx[]
		printf("<-- rx[0-8] : %d bytes \n", p+1);
		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", rx[k]);
		}
		printf(": CRC(0x%02X)\n", crc);

		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", rx[k+8]);
		}
		printf("\n");


		if (i > 0)
		{
			if (crc != rx[p])
			{
				fprintf(stderr, "\n!!  error in crc check (0x%02X) : rx[%d] = 0x%02X\n", 
						crc, p, rx[p]);

				unsigned char err_code = ERR_CRC;
				if (rx[0] != dev_list[i])
				{
					err_code |= ERR_CMD;
				}
				//send_error_code(tx[0], err_code);

				//return 0;
				cfg->CRC_status = 0;
			}
			else
			{
				cfg->CRC_status = 1;
			}

			printf("    recv_data_size = %d\n", recv_data_size);
			memcpy(msg_buff, &rx[8], recv_data_size);
		}
		
		usleep(cfg->spi_delay);
	}

	//printf("rx[0] = %c (0x%02X)\n", rx[0], rx[0]);

	return recv_data_size;
}
*/


// ----------------------------------------------------------------------------
// for sending ESS System BMS data to micom
//
int micom_send_data (unsigned char *msg_buff, int buff_size);
{
	unsigned char dev_type[] = { 0x7E, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
	unsigned char ecu_type[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
	unsigned char cmd_type[] = { 0x00, 0x01, 0x02, 0x03 };

	/* check input parameter */
	if (dev > sizeof(dev_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in dev input (dev = %d)\n", dev);
		dev = 0;
	}

	if (ecu > sizeof(ecu_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in ecu input (ecu = %d)\n", ecu);
		ecu = 0;
	}

	if (dev > sizeof(dev_type) - 1)
	{
		fprintf(stderr, "micom_get_data() : error in cmd input (cmd = %d)\n", cmd);
		dev = 0;
	}

	int data_size = 0;

	switch (ecu)
	{
		case SPI_ECU_INV :
			data_size = cfg->ecu_data_size[ECU_TYPE_INV] + 2;
			break;
		case SPI_ECU_PCS :
			data_size = cfg->ecu_data_size[ECU_TYPE_PCS] + 2;
			break;
		case SPI_ECU_LIP1 :
		case SPI_ECU_LIP2 :
			data_size = cfg->ecu_data_size[ECU_TYPE_LIP] + 2;
			break;
		case SPI_ECU_ESS :
			data_size = cfg->ecu_data_size[ECU_TYPE_ESS] + 2;
			break;
		case SPI_ECU_PRU :
			data_size = cfg->ecu_data_size[ECU_TYPE_PRU] + 2;
			break;
		case SPI_ECU_NONE :
			if (dev == SPI_DEV_MICOM)
			{
				data_size = 0;
				for (int i=0; i<MAX_SPU; i++)
				{
					if (cfg->SPU_info[i])
					{
						data_size += cfg->spu_data_size[i];
					}
				}
				data_size += cfg->num_HIP * (cfg->ecu_data_size[ECU_TYPE_HIP] + 2);
			}
			else if (dev >= SPI_DEV_SPU1 && dev <= SPI_DEV_SPU4)
			{
				int spu_id = dev - SPI_DEV_SPU1;
				data_size = cfg->spu_data_size[spu_id];
			}
			else if (dev >= SPI_DEV_HIP1 && dev <= SPI_DEV_HIP2)
			{
				data_size = cfg->ecu_data_size[ECU_TYPE_HIP] + 2;
			}
			break;
	}


	/* check buff_size */
	if (buff_size < data_size)
	{
		fprintf(stderr, "buff_size (%d) < data_size (%d) in micom_get_data()\n", 
				buff_size, data_size);
		return 0;
	}


	unsigned char dev_list[] = { DEV_NULL, 0x00, DEV_NULL};
	unsigned char cmd_list[] = { 0x00, 0x00, 0x00 };
	unsigned int size_list[] = { 0, 0, 0 };

	dev_list[1] = dev_type[dev];
	cmd_list[1] = ecu_type[ecu] | cmd_type[cmd];
	size_list[1] = data_size;
 

	/* spi comm */
	unsigned char tx[MAX_BUFF_SIZE] = {0,};
	unsigned char rx[MAX_BUFF_SIZE] = {0,};
	unsigned char crc;

	int recv_data_size = 0;

	for (int i=0; i<2; i++)
	{
		/* tx[] = devicei(1) cmd(1), data_size(2), next_cmd(4), crc(1) */
		int p = 0;

		tx[p++] = dev_list[i];
		tx[p++] = cmd_list[i];
		tx[p++] = (size_list[i] >> 8) & 0xFF;
		tx[p++] = size_list[i] & 0xFF;

		tx[p++] = dev_list[i+1];
		tx[p++] = cmd_list[i+1];
		tx[p++] = (size_list[i+1] >> 8) & 0xFF;
		tx[p++] = size_list[i+1] & 0xFF;

		p += size_list[i];
		tx[p] = crc_check(tx, p);
		
		// tx[]
		printf("--> tx[0-8] : %d bytes \n", p+1);
		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", tx[k]);
		}
		printf(": CRC(0x%02X)\n", tx[p]);

		spi_comm(tx, rx, p+1);
		//printf(" spi_comm() called !\n");

		recv_data_size = (rx[2] << 8) + rx[3];
		if (recv_data_size <= (p-8))
		{
			p = recv_data_size + 8;
		}
		else
		{
			/* error case */
			recv_data_size = p - 8;
		}
		//printf(" p = %d \n", p);
		unsigned char crc = crc_check(rx, p);

		// rx[]
		printf("<-- rx[0-8] : %d bytes \n", p+1);
		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", rx[k]);
		}
		printf(": CRC(0x%02X)\n", crc);

		printf("              ");
		for (int k=0; k<8; k++)
		{
			printf("0x%02X ", rx[k+8]);
		}
		printf("\n");


		if (i > 0)
		{
			if (crc != rx[p])
			{
				fprintf(stderr, "\n!!  error in crc check (0x%02X) : rx[%d] = 0x%02X\n", 
						crc, p, rx[p]);

				unsigned char err_code = ERR_CRC;
				if (rx[0] != dev_list[i])
				{
					err_code |= ERR_CMD;
				}
				//send_error_code(tx[0], err_code);

				//return 0;
				cfg->CRC_status = 0;
			}
			else
			{
				cfg->CRC_status = 1;
			}

			printf("    recv_data_size = %d\n", recv_data_size);
			memcpy(msg_buff, &rx[8], recv_data_size);
		}
		
		usleep(cfg->spi_delay);
	}

	//printf("rx[0] = %c (0x%02X)\n", rx[0], rx[0]);

	return recv_data_size;
}

