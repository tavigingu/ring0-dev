#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // open
#include <unistd.h>     // close
#include <stdint.h>     // int32_t
#include <sys/ioctl.h>  // ioctl
#include <errno.h>
#include <string.h>

// IOCTL command definitions (must match the ones in your kernel driver)
#define WR_DATA _IOW('a', 'a', int32_t*) 
#define RD_DATA _IOR('a', 'b', int32_t*)  

int main() {
    int fd;
    int32_t write_val = 123;
    int32_t read_val;

    fd = open("/dev/my_device", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/my_device");
        return EXIT_FAILURE;
    }
    printf("Device opened successfully\n");

    //send data to the driver via ioctl (write)
    if (ioctl(fd, WR_DATA, &write_val) < 0) {
        perror("Failed to write data via ioctl");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Value %d sent to driver via ioctl\n", write_val);

    //read data from the driver via ioctl (read)
    if (ioctl(fd, RD_DATA, &read_val) < 0) {
        perror("Failed to read data via ioctl");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Value read from driver via ioctl: %d\n", read_val);

    //close the device
    close(fd);
    printf("Device closed\n");

    return EXIT_SUCCESS;
}
