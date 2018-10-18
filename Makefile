all:
	#$(CC) -Wall -Werror ess_raspi.c -O2 -lpthread -o ess_raspi
	$(CC) -Wall ess_raspi.c mpu_conio.c -O2 -lpthread -o ess_raspi

clean:
	$(RM) -rf ess_raspi
