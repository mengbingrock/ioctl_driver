#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "../dev/ioctl.h"

#define IOCTL_DRIVER_NAME "/dev/ioctl"

int open_driver(const char* driver_name);
void close_driver(const char* driver_name, int fd_driver);

int open_driver(const char* driver_name) {

    printf("* Open Driver\n");

    int fd_driver = open(driver_name, O_RDWR);
    if (fd_driver == -1) {
        printf("ERROR: could not open \"%s\".\n", driver_name);
        printf("    errno = %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	return fd_driver;
}


int ioctl_get_msg(int file_desc){
  int ret_val;
  char message[100];

  //ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

}


void close_driver(const char* driver_name, int fd_driver) {

    printf("* Close Driver\n");

    int result = close(fd_driver);
    if (result == -1) {
        printf("ERROR: could not close \"%s\".\n", driver_name);
        printf("    errno = %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}


int main(void) {

  int fd_ioctl = open_driver(IOCTL_DRIVER_NAME);
  uint32_t value;
  int tmp = 0;
	if (ioctl(fd_ioctl, IOCTL_BASE_GET_MUIR, &value) < 0) {
			perror("Error ioctl PL_AXI_DMA_GET_NUM_DEVICES");
			exit(EXIT_FAILURE);
	}

  printf("Value is %u\n", value);

  printf("The address of tmp is %p \n",&tmp);

        if (ioctl(fd_ioctl, IOCTL_BASE_PAGE_TABLE, &tmp) < 0) {
                        perror("Error ioctl PAGE TABLE");
                        exit(EXIT_FAILURE);
        } 

	close_driver(IOCTL_DRIVER_NAME, fd_ioctl);

	return EXIT_SUCCESS;
}


