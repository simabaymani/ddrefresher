/*
 * Test program for kernel module
 * Use to perform ioctl commands by simply writing an input number
 * to the device, and then reading it back.
 * Build it separately using 
 * gcc -o test_ioctl test_ioctl.c
 * 
 * Usage:
 * 	> ./test_ioctl <number>
 *
 * */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h> //strtol
#include <fcntl.h> //open
#include <unistd.h> //close

#define TESTMOD_IOCTL_MAGIC ('t')

#define TESTMOD_IOCTL_W _IOW( TESTMOD_IOCTL_MAGIC, 1, int32_t *)
#define TESTMOD_IOCTL_R _IOR( TESTMOD_IOCTL_MAGIC, 2, int32_t *)

int main(int argc, char ** argv)
{
	int fd;
	int32_t val;
	int arg = 0;

	printf("Open device filer\n");
	fd = open("/dev/dyn_testdev_dev", O_RDWR);
	if(fd < 0) {
		printf("Error opening device file.\n");
		return 0;
	}

	if(argc < 2 ) {
		printf("Too few arguments (need one).\n");
		return 0;
	}

	arg = (int32_t) strtol(argv[1], NULL, 10);
	printf("Writing input to driver\n");
	ioctl(fd, TESTMOD_IOCTL_W, (int32_t*) &arg); 

	printf("Reading data from driver\n");
	ioctl(fd, TESTMOD_IOCTL_R, (int32_t*) &val);
	printf("val = %d\n", val);

	close(fd);
}
