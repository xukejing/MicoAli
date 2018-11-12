//udp转mysql
//串口数据交互
#include <sys/types.h>
#include <sys/socket.h>
#include<pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>  
#include <netdb.h> 
#include <errno.h> 
#include <mysql/my_global.h>
#include <mysql/mysql.h>

#include <fcntl.h>
#include <termios.h>

#define err_sys(msg) \
	do { perror(msg); exit(-1); } while(0)
#define err_exit(msg) \
	do { fprintf(stderr, msg); exit(-1); } while(0)

void *thread_udp_server(void *arg);
void *thread_mysql_cilent(void *arg);
void *thread_tty_server(void *arg);
static int is_udp = 0;
static int is_tty = 0;
static int mysql_cilent_idle = 1;
float udp_float[2] = { 0 };
int catID = 0;
int countCat = 0;
int main()
{
	pthread_t tid_tty_server;
	char* p_tty_server = NULL;

	pthread_create(&tid_tty_server, NULL, thread_tty_server, NULL);

	pthread_t tid_udp_server;
	char* p_udp_server = NULL;

	pthread_create(&tid_udp_server, NULL, thread_udp_server, NULL);

	pthread_t tid_mysql_cilent;
	char* p_mysql_cilent = NULL;

	pthread_create(&tid_mysql_cilent, NULL, thread_mysql_cilent, NULL);
	sleep(5);
	pthread_join(tid_mysql_cilent, (void **)&p_mysql_cilent);;
	pthread_join(tid_udp_server, (void **)&p_udp_server);
	pthread_join(tid_tty_server, (void **)&p_tty_server);
	//printf("message: %s\n", p_udp_server);

	return 0;
}
void *thread_tty_server(void *arg)
{
	int fd = -1;
	fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		perror("Open Serial Port Error!\n");
		
	}

	struct termios options;
	tcgetattr(fd, &options);
	//115200, 8N1
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);
	unsigned char rx_buffer[256];
	while (1) 
	{
		if (is_tty == 1)
		{
			write(fd, "\rAT+TTS=get out right now\r", 27);
			is_tty = 0;
		}
		int rx_length = read(fd, (void*)rx_buffer, 255);
		if (rx_length > 0)
		{
			//Bytes received
			rx_buffer[rx_length] = '\0';
			printf("%i bytes read : %s\n", rx_length, rx_buffer);
			if (rx_buffer[rx_length - 3] == '1'&&rx_buffer[rx_length - 4] == ':'&&rx_buffer[rx_length - 5] == 'T'&&rx_buffer[rx_length - 6] == 'N')
			{
				printf("led on \n");
				write(fd, "\rAT+TTS=light on\r", 18);
				system("echo 255 > /sys/devices/platform/gpio-leds/leds/status_led/brightness");
			}
			if (rx_buffer[rx_length - 3] == '2'&&rx_buffer[rx_length - 4] == ':'&&rx_buffer[rx_length - 5] == 'T'&&rx_buffer[rx_length - 6] == 'N')
			{
				printf("led off \n");
				write(fd, "\rAT+TTS=light off\r", 19);
				system("echo 0 > /sys/devices/platform/gpio-leds/leds/status_led/brightness");
			}
			if (rx_buffer[rx_length - 3] == '3'&&rx_buffer[rx_length - 4] == ':'&&rx_buffer[rx_length - 5] == 'T'&&rx_buffer[rx_length - 6] == 'N')
			{
				if (countCat >= 1)
				{
					write(fd, "\rAT+TTS=cat has come\r", 22);
				}
				else
					write(fd, "\rAT+TTS=cat has not come\r", 26);
				printf("cats count is send to tty\n");
			}
			if (rx_buffer[rx_length - 3] == '4'&&rx_buffer[rx_length - 4] == ':'&&rx_buffer[rx_length - 5] == 'T'&&rx_buffer[rx_length - 6] == 'N')
			{
				countCat = 0;
				printf("count is set 0 \n");
				write(fd, "\rAT+TTS=cats number is 0\r", 26);
			}
		}
	}
}

void *thread_udp_server(void *arg)
{
	time_t timep;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6000);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	printf("udp server is started!\n");
	char buff[64];
	struct sockaddr_in clientAddr;
	int n;
	unsigned int len = sizeof(clientAddr);
	while (1)
	{
		n=recvfrom(sock, buff, 64, 0, (struct sockaddr*)&clientAddr, &len);
		if (n > 0)
		{
			float *floatBuff = (float *)buff;//char转float
			time(&timep);
			printf("%f\t%f\t%s\n", floatBuff[0], floatBuff[1],  ctime(&timep));
			if (mysql_cilent_idle == 1)
			{
				udp_float[0] = floatBuff[0];
				udp_float[1] = floatBuff[1];
				printf("udp data is recved!\n");
				is_udp = 1;
				is_tty = 1;
			}
			else
				printf("mysql cilent is busy!\n");
			
		}
	}
}

void *thread_mysql_cilent(void *arg)
{

	while (1)
	{
		switch (is_udp)
		{
		case 1:
			char query[256];
			time_t timep;
			//printf("MySQL client version: %s\n", mysql_get_client_info());
			MYSQL *mysql;
			mysql = mysql_init(NULL);
			if (!mysql_real_connect(mysql, "服务器地址", "用户名", "密码", "homeiot", 3306, NULL, 0))
			{
				printf("error in home_iot connecting!\n");
				//exit(0);
				break;
			}
			else
			{
				printf("DB of home_iot is connected!\n");
			}
			mysql_cilent_idle = 0;
			printf("inserting data to Table...\n");
			time(&timep);
			sprintf(query, "delete from ctest where id = '%04d'", catID);
			mysql_query(mysql, query);
			sprintf(query, "insert into ctest values ('%04d','1','%.2f','%.2f','%s')", catID, udp_float[0], udp_float[1], ctime(&timep));
			catID++;
			countCat++;
			mysql_query(mysql, query);
			printf("mysql cilent is idle!\n");
			is_udp = 0;
			mysql_close(mysql);
			mysql_cilent_idle = 1;
			break;
		default:
			break;
		}
	}
}

