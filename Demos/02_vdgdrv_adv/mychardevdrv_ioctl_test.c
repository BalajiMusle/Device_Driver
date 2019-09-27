#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "mychardevdrv.h"

int main()
{
	int fd, ret, flag;
	fd = open("/dev/vdg_chrdev", O_RDWR);
	if(fd < 0)
	{
		perror("open() failed");
		_exit(1);
	}
	printf("open() called : ret = %d\n", fd);
	getchar();

	/*
	ret = ioctl(fd, VDG_CLEAR);
	printf("ioctl() called : ret = %d\n", ret);
	getchar();
	*/

	/*
	flag = VDG_UPPER;
	ret = ioctl(fd, VDG_CHANGE_CASE, &flag);
	printf("ioctl() called : ret = %d\n", ret);
	if(ret < 0)
		perror("ioctl() failed");
	getchar();
	*/

	flag = VDG_LOWER;
	ret = ioctl(fd, VDG_CHANGE_CASE, &flag);
	printf("ioctl() called : ret = %d\n", ret);
	if(ret < 0)
		perror("ioctl() failed");
	getchar();
	
	close(fd);
	printf("close() called : fd = %d\n", fd);
	return 0;
}

