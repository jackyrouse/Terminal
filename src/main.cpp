/*
 #define Dameon true
 #define MAXFILE 65535
 */
#include "main.h"

MainProgram::MainProgram()
{
}

MainProgram::~MainProgram()
{

}

int MainProgram::init()
{
	return 1;
}

int MainProgram::start()
{
	//--从文件初始化所有变量
	intervalseconds = read_profile_int("Interval","seconds",15,"/root/terminal.ini");
	printf("seconds=%d\n",intervalseconds);

	if(!read_profile_string("StatusPosition","Position",CurrentRecordFilename, 5,"00","/root/terminal.ini"))
	{
		printf("Read ini file failed, Programme exit!\n");
		return -1;
	}
	else
	{
		printf("Position=%s\n",CurrentRecordFilename);
	}

	if(!read_profile_string("LoginIP","IP",LogHost, 20,"192.168.1.1","/root/terminal.ini"))
	{
		printf("Read ini file failed, Programme exit!\n");
		return -1;
	}
	else
	{
		printf("LoginIP=%s\n",LogHost);
	}

	if(!read_profile_string("device","Serials",Serdevice,10,"ttyS1","/root/terminal.ini"))
	{
		printf("Read ini file failed, Programme exit!\n");
		return -1;
	}
	else
	{
		printf("device=%s\n",Serdevice);
	}

	if(!read_profile_string("Version","Currentv",Version,10,"0.1","/root/terminal.ini"))
	{
		printf("Read ini file failed, Programme exit!\n");
		return -1;
	}
	else
	{
		printf("Currentv=%s\n",Version);
		char* p;
		p = strtok(Version, ".");
		VersionH = atoi(p);
		p = strtok(NULL, ".");
		VersionL = atoi(p);
	}

	LogPort = read_profile_int("LoginPORT","PORT",5000,"/root/terminal.ini");
	printf("LoginPORT=%d\n",LogPort);

	SerSpeed = read_profile_int("Serial","Speed",4800,"/root/terminal.ini");
	printf("SerSpeed=%d\n",SerSpeed);

	timeouts = read_profile_int("ReadTimeout","Timeout",500,"/root/terminal.ini");
	printf("timeousts=%d\n",timeouts);
	//--初始化结束

	//--获取MAC地址与2字节加密数据
	memset(MACENC, 0, 8);
	char device[10];
	strcpy(device,"eth0"); //teh0是网卡设备名
	unsigned char macaddr[ETH_ALEN]; //ETH_ALEN（6）是MAC地址长度
	struct ifreq req;
	int err, i;

	int s = socket(AF_INET, SOCK_DGRAM, 0); //internet协议族的数据报类型套接口
	strcpy(req.ifr_name, device); //将设备名作为输入参数传入
	err = ioctl(s, SIOCGIFHWADDR, &req); //执行取MAC地址操作
	close(s);
	if (err != -1)
	{
	/*	memcpy(macaddr, req.ifr_hwaddr.sa_data, ETH_ALEN); //取输出的MAC地址
		for(i=0;i<ETH_ALEN;i++)
		{
			MACENC[2+i] = macaddr[i];
			printf("%02x：",macaddr[i]);
		}*/
	}
	MACENC[0] = 0x00;
	MACENC[1] = 0x00;
	MACENC[2] = 0xAA;
	MACENC[3] = 0xBB;
	MACENC[4] = 0xCC;
	MACENC[5] = 0xDD;
	MACENC[6] = 0xEE;
	MACENC[7] = 0xF9;
	printf("\n");
	//--end获取MAC地址与2字节加密数据

	m_NetworkConfig.Init(this);

	//
	//strcpy(m_NetworkConfig.wifi_ssid, "TESTTEST");
	//strcpy(m_NetworkConfig.wifi_key, "87654321");
	//m_NetworkConfig.uci_wifi_config_set();
	//

	strcpy(m_CommHelper.CurrentRecordPos, CurrentRecordFilename);
	if (m_CommHelper.Open(this, Serdevice, SerSpeed) < 0)
	{
		printf("Device open failed. Programme exit!\n");
		exit(1);
	}
	else
	{
		printf("Comm Speed is %d.\n", m_CommHelper.Speed);
		m_CommHelper.timeouts = timeouts;
	}

	//设置m_tcpClient属性
	strcpy(m_tcpClient.m_remoteHost, LogHost); //

	m_tcpClient.m_port = LogPort;
	m_tcpClient.Open(this);

	if (m_tcpClient.ConnectServer())
	{
		fprintf(stdout, "TCP建立连接成功!\n");
		//--将获取实际服务器IP＆PORT的指令加入发送队列
		memset(TmpData, 0, 20);
		TmpData[0] = 0x55;
		memcpy(&TmpData[1], MACENC, 8);
		TmpData[9] = 0x00;
		TmpData[10] = 0xF0;
		TmpData[11] = 0x01;
		TmpData[12] = VersionH;
		TmpData[13] = VersionL;
		TmpData[14] = 0x00;
		TmpData[15] = CRC8(&TmpData[1],14);
		TmpData[16] = 0xAA;
		m_tcpClient.AddToSendQueue((char*)TmpData, 17);
		m_tcpClient.Headarray[0] = 0x55;
		memcpy(&(m_tcpClient.Headarray[1]), MACENC, 8);
		//--加入完毕
	}
	else
	{
		fprintf(stdout, "TCP建立连接失败!.\n");
		//return -1;
	}

	m_RecSSIDPWDaemon.netconfigptr = &m_NetworkConfig;
	m_RecSSIDPWDaemon.CreateRevSSIDPWThread();

	return 0;
}

int background = 0; /* 后台运行标记  */
int Outlog = 0; //后台运行时保存输出信息
pthread_cond_t m_exitEvent;
pthread_mutex_t mutex; /*互斥锁*/

int main(int argc, char *argv[])
{

	MainProgram m_MainProgram;

	//初始化并解释程序的启动参数
	extern char *optarg;
	int c;

	while ((c = getopt(argc, argv, "e:p:s:bh")) != EOF)
	{
		switch (c)
		{
		case 'e':
			strcpy(m_MainProgram.Serdevice, optarg);
			break;
		case 'p':
			m_MainProgram.RemotePort = atoi(optarg);
			break;
		case 's':
			strcpy(m_MainProgram.RemoteHost, optarg);
			break;
		case 'b':
			background = 1;
			break;
		case '?':
		case 'h':
			printf(
					"Usage: %s [-e <Serdevice>] [-p <RemotePort>] [-s <RemoteHost>] [-b <Background>]\n",
					argv[0]);
			exit(0);
		}
	}

	if (background)
	{
		//创建退出事件句柄
		pthread_mutex_init(&mutex, NULL);
		pthread_cond_init(&m_exitEvent, NULL);

		/*		int daemon(int nochdir, int noclose) 参数：
		 当 nochdir为零时，当前目录变为根目录，否则不变；
		 当 noclose为零时，标准输入、标准输出和错误输出重导向为/dev/null，也就是不输出任何信 息，否则照样输出。
		 返回值：
		 deamon()调用了fork()，如果fork成功，那么父进程就调用_exit(2)退出，所以看到的错误信息 全部是子进程产生的。如果成功函数返回0，否则返回-1并设置errno。
		 */
		if (daemon(1, 0) != 0)
		{
			perror("Daemon error!\n");
			exit(1);
		}

		char device[10];
		strcpy(device, "ttyATH0");
		strcpy(m_MainProgram.Serdevice, device);
		/*		for (int i = '0'; i <= '9'; i++) { //自动查找一个可用的设备
		 device[11] = i;
		 //printf ("可用的devce：[%s]\n",device.c_str()) ;
		 if (access(device, F_OK) != -1) {
		 printf("可用的devce：[%s]\n", device + 5);
		 strcpy(m_MainProgram.Serdevice, device + 5);
		 break;
		 }
		 }*/
	}

	m_MainProgram.start();

	if (!background)
	{
		char usercmd[128];
		while (true)
		{
			cin.getline(usercmd, 127);
			switch (usercmd[0])
			{
			case 'q':
			case 'Q':
			{
				if (m_MainProgram.m_tcpClient.Close() == 1)
					fprintf(stdout, "Exit success.\n");
				return 0;
			}
			case '!':
			{
				if (strlen(usercmd) < 2)
				{
					fprintf(stdout, "没有ASCII数据要发送!\n");
					break;
				}
				char Strtmp[128];
				strncpy(Strtmp, usercmd + 1, strlen(usercmd) - 1);
				Strtmp[strlen(usercmd) - 1] = 0;
				m_MainProgram.m_tcpClient.SendData(Strtmp, strlen(Strtmp));
				break;
			}
			case '@':
			{
				if (strlen(usercmd) < 2)
				{
					fprintf(stdout, "没有Hex数据要发送!\n");
					break;
				}
				char SendBuf[64] =
				{ 0 };
				char StrTmp[3];
				int tmp = 0, count = 0;
				char *pusercmd = usercmd;
				pusercmd += 1;
				for (int i = 1; i < (int) strlen(usercmd); i += 3)
				{
					strncpy(StrTmp, pusercmd, 2);
					StrTmp[2] = 0;
					sscanf(StrTmp, "%02X", &tmp);
					pusercmd += 3;
					SendBuf[count++] = tmp;
				}
				m_MainProgram.m_tcpClient.SendData(SendBuf, count);

			}
				break;
			case '#':
			{
				if (strlen(usercmd) < 2)
				{
					fprintf(stdout, "没有ASCII数据要发送到串口!\n");
					break;
				}
				char Strtmp[128];
				strncpy(Strtmp, usercmd + 1, strlen(usercmd) - 1);
				Strtmp[strlen(usercmd) - 1] = 0;
				m_MainProgram.m_CommHelper.SendData(Strtmp, strlen(Strtmp));
				break;
			}
			case '$':
			{
				if (strlen(usercmd) < 2)
				{
					fprintf(stdout, "没有Hex数据要发送到串口!\n");
					break;
				}
				char SendBuf[64] =
				{ 0 };
				char StrTmp[3];
				int tmp = 0, count = 0;
				char *pusercmd = usercmd;
				pusercmd += 1;
				for (int i = 1; i < (int) strlen(usercmd); i += 3)
				{
					strncpy(StrTmp, pusercmd, 2);
					StrTmp[2] = 0;
					sscanf(StrTmp, "%02X", &tmp);
					pusercmd += 3;
					SendBuf[count++] = tmp;
				}
				m_MainProgram.m_CommHelper.SendData(SendBuf, count);
				m_MainProgram.m_CommHelper.ExtractData(SendBuf, count);
			}
				break;
			default:
				fprintf(stdout, "命令未知!请输入 h | H 查看帮助.\n");
				break;
			}
		}
	}
	else
	{
		//sleep(1);
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&m_exitEvent, &mutex);
		pthread_mutex_unlock(&mutex);
		fprintf(stdout, "收到退出信号.\n");
		pthread_cond_destroy(&m_exitEvent);
	}

	return 0;
}
