/*
 * RevSSIDPWDaemon.cpp
 *
 *  Created on: Dec 4, 2014
 *      Author: jacky
 */

#include "RevSSIDPWDaemon.h"
#include "main.h"

RevSSIDPWDaemon::RevSSIDPWDaemon()
{
	// TODO Auto-generated constructor stub

}

RevSSIDPWDaemon::~RevSSIDPWDaemon()
{
	// TODO Auto-generated destructor stub
}

void *RevSSIDPWDaemon::RevThreadFunc(void* lparam)
{
	RevSSIDPWDaemon *RevDaemon;
	RevDaemon = (RevSSIDPWDaemon *) lparam;
	int lisenfd, connfd;
	struct sockaddr_in ser_addr, cli_addr;
	socklen_t cli_len;
	int len = 0;
	char recv_buf[1024];
	char send_buf[1024];
	char SSID_buf[50];
	char PW_buf[50];
	lisenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lisenfd)
	{
		fprintf(stdout, "Failed to socket");
		return NULL;
	}
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(5050);  //这里输入服务器端口号
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY表示本机IP
	if (-1 == bind(lisenfd, (struct sockaddr *) &ser_addr, sizeof(ser_addr)))
	{
		fprintf(stdout, "Failed to bind");
		return NULL;
	}
	printf("Now listen...\n");
	if (-1 == listen(lisenfd, 10))
	{
		fprintf(stdout, "Failed to listen");
		return NULL;
	}
	while (1)
	{
		memset(&cli_addr, 0, sizeof(cli_addr));
		cli_len = sizeof(cli_addr);
		fprintf(stdout, "Now accept...\n");
		connfd = accept(lisenfd, (struct sockaddr *) &cli_addr, &cli_len);
		if (-1 == connfd)
		{
			fprintf(stdout, "Failed to accept");
		}
		else
		{
			memset(recv_buf, 0, sizeof(recv_buf));
			fprintf(stdout, "Now recv...\n");
			len = recv(connfd, recv_buf, sizeof(recv_buf), 0);
			if (-1 == len)
			{
				fprintf(stdout, "Failed to recv");
			}
			else
			{
				fprintf(stdout, "Recv from client is %s\n", recv_buf);

				//Analyze SSID and PASSWORD
				memset(SSID_buf, 0, 50);
				memset(PW_buf, 0, 50);
				string ssidandpwstr = recv_buf;
				Json::Reader reader;
				Json::Value value;
				if(reader.parse(ssidandpwstr,value))
				{
					strcpy(SSID_buf, value["SSID"].asString().c_str());
					strcpy(PW_buf, value["PASSWORD"].asString().c_str());
				}
				//write to wireless
				strcpy(RevDaemon->netconfigptr->wifi_ssid, SSID_buf);
				strcpy(RevDaemon->netconfigptr->wifi_key, PW_buf);
				//restart wifi client network
				RevDaemon->netconfigptr->uci_wifi_config_set();
				RevDaemon->netconfigptr->NetworkRestart();

				fprintf(stdout, "Now send...\n");
				memset(send_buf, 0, sizeof(send_buf));
				sprintf(send_buf, "SSIDPASSWORDreceiveok!");
				len = send(connfd, send_buf, strlen(send_buf), 0);
				if (-1 == len)
				{
					fprintf(stdout, "Failed to send");
				}
				else
				{
					fprintf(stdout, "Restart wifi client OK!\n");
					close(connfd);
				}
			}
		}
	}
	close(lisenfd);
	return NULL;
}

bool RevSSIDPWDaemon::CreateRevSSIDPWThread()
{
	int ret = pthread_create(&m_RevThreadHande, NULL, RevThreadFunc,
			(void*) this);
	if (ret)
	{
		fprintf(stdout, "Create RecSSIDPWThreadFunc thread error!\n");
		return false;
	}
	return true;
}
