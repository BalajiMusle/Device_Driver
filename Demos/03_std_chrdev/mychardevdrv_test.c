#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main()
{
	char buf[64] = "Hello DESD!!";
	int fd, nfd, ret;
	fd = open("/dev/vdg_chrdev", O_RDWR);
	if(fd < 0)
	{
		perror("open() failed");
		_exit(1);
	}
	printf("open() called : ret = %d\n", fd);
	getchar();
	
	ret = write(fd, buf, strlen(buf));
	printf("write() called : ret = %d\n", ret);
	getchar();
	
	strcpy(buf, "Hello Sunbeam!!!");
	ret = write(fd, buf, strlen(buf));
	printf("write() called : ret = %d\n", ret);
	getchar();

	ret = lseek(fd, 0, SEEK_SET);
	printf("lseek() called : ret = %d\n", ret);
	getchar();
	
	ret = read(fd, buf, sizeof(buf));
	printf("read() called : ret = %d\n", ret);
	if(ret > 0)
	{
		buf[ret] = '\0';
		printf("read buf = %s\n", buf);
	}
	getchar();

	ret = lseek(fd, -3995, SEEK_END);
	printf("lseek() called : ret = %d\n", ret);
	getchar();
	
	ret = read(fd, buf, sizeof(buf));
	printf("read() called : ret = %d\n", ret);
	if(ret > 0)
	{
		buf[ret] = '\0';
		printf("read buf = %s\n", buf);
	}
	getchar();

	close(fd);
	printf("close() called : fd = %d\n", fd);
	return 0;
}

