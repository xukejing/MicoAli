//udp转mysql
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

#define err_sys(msg) \
	do { perror(msg); exit(-1); } while(0)
#define err_exit(msg) \
	do { fprintf(stderr, msg); exit(-1); } while(0)

void *thread_udp_server(void *arg);
void *thread_mysql_cilent(void *arg);
static int is_udp = 0;
static int mysql_cilent_idle = 1;
float udp_float[2] = { 0 };
int catID = 0;
int main()
{
	pthread_t tid_udp_server;
	char* p_udp_server = NULL;

	pthread_create(&tid_udp_server, NULL, thread_udp_server, NULL);

	pthread_t tid_mysql_cilent;
	char* p_mysql_cilent = NULL;

	pthread_create(&tid_mysql_cilent, NULL, thread_mysql_cilent, NULL);
	sleep(5);
	pthread_join(tid_mysql_cilent, (void **)&p_mysql_cilent);;
	pthread_join(tid_udp_server, (void **)&p_udp_server);

	//printf("message: %s\n", p_udp_server);

	return 0;
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
			if (!mysql_real_connect(mysql, "IP address", "user-name", "passwdword", "homeiot", 3306, NULL, 0))
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

