
#ifndef _SPI_COMM_H_
#define _SPI_COMM_H_

//#include "mpu_config.h"
//#include "spi_emul.h"

#define SPU_BUFF_SIZE (256)
#define MAX_BUFF_SIZE (2048)


typedef enum spi_dev_t
{
	SPI_DEV_MICOM	= 0,
	SPI_DEV_SPU1	= 1,
	SPI_DEV_SPU2	= 2, 
	SPI_DEV_SPU3	= 3,
	SPI_DEV_SPU4	= 4,
	SPI_DEV_HIP1	= 5,
	SPI_DEV_HIP2	= 6
} spi_dev_t;

typedef enum spi_ecu_t
{
	SPI_ECU_NONE	= 0,
	SPI_ECU_INV		= 1,
	SPI_ECU_PCS		= 2,
	SPI_ECU_LIP1	= 3,
	SPI_ECU_LIP2	= 4,
	SPI_ECU_ESS		= 5,
	SPI_ECU_PRU		= 6
} spi_ecu_t;

typedef enum spi_cmd_t
{
	SPI_CMD_STATUS	= 0,
	SPI_CMD_DATA	= 1,
	SPI_CMD_ERROR	= 2
} spi_cmd_t;


int spi_open();
void spi_close();

//int spu_comm(unsigned char *rx, int buff_size, mpu_config_t *cfg);
//int micom_get_data (int dev, int ecu, int cmd, unsigned char *msg_buff, int buff_size, mpu_config_t *cfg);

int micom_send_data (unsigned char *msg_buff, int buff_size);

#endif



