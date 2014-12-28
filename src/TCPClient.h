/*
 * TCPClient.h
 *
 *  Created on: 2014-7-17
 *      Author: root
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>

#include "unit.h"
#include "NetworkConfig.h"

#define SEND_TRY_TIMES 3 ;
#define SEND_TIME_OUT_MS 500 ; //ms

using namespace std;

struct ServerData
{
	friend bool operator <(ServerData n1, ServerData n2)
	{
		return n1.priority > n2.priority;
	}
	int priority;
	char *data;
	int data_len;
	ServerData()
	{
		this->priority = 10;
		this->data_len = 0;
		this->data = NULL;
	}
	void free()
	{
		if (data != NULL)
		{
			delete (data);
			data = NULL;
		}
		data_len = 0;
	}
};

class MainProgram;

class TCPClient
{
public:
	TCPClient();
	virtual ~TCPClient();
public:
	//打开客户端socket
	int Open(MainProgram *lp); //打开TCP
	bool CreateRevThread();
	//关闭客户端socket
	bool Close();
	//与服务器端建立连接
	bool Connect();
	//向服务器端发送数据
	bool SendData(const char * buf, int len);
	bool getConnectionStatus();
	void AddToSendQueue(const char * data, int data_len, int priority = 10);
	void AddToRevQueue(const char * data, int data_len, int priority = 10);
	bool ReConnect();
	bool ConnectServer();
	void setExitFlag(bool flag);
	int MsgProcess(const ServerData* msg);
	int ExtractData(const char *data, int data_len);
	void uploadstatusfile();
	char GetOnlineMode();

	int IntervalSeconds;
	int fileposition;

	char statusarry[32];
	char Headarray[9];
public:
	MainProgram * m_MainProgram; //引用
	//远程主机IP地址
	char m_remoteHost[16];
	//远程主机端口
	int m_port;


	priority_queue<ServerData> RevDataQueue;
	priority_queue<ServerData> SendDataQueue;

	pthread_mutex_t SendDataQueuemutex; /*初始化互斥锁*/
	pthread_mutex_t RevDataQueuemutex; /*初始化互斥锁*/

private:
	//通讯Socket句柄
	int m_socket;
	bool ExitFlag;
	bool ConnectionStatus;
	bool RevThreadFuncStatus;
	bool RealConnected;

	pthread_mutex_t MsgProcessThreadmutex; /*初始化互斥锁*/
	pthread_cond_t MsgProcessThreadEvent; //通讯线程退出事件句柄

	pthread_mutex_t SendQueueThreadmutex; /*初始化互斥锁*/
	pthread_cond_t SendQueueThreadEvent; //通讯线程退出事件句柄

	pthread_t m_tcpThreadHandle;
	pthread_t m_MsgProcessThreadHandle; //通讯线程句柄
	pthread_t m_SendQueueThreadHandle; //通讯线程句柄
	pthread_t m_TimerQueryThreadHandle;
private:
	//通讯线程函数
	static void *SocketThreadFunc(void * lparam);
	static void *SendQueueThreadFunc(void * lparam);
	static void *MsgProcessThreadFunc(void * lparam);
	//定时查询设备状态线程，定时向RevDataQueue队列中加入查询命令
	static void *TimerQueryThreadFunc(void * lparam);
};

#endif /* TCPCLIENT_H_ */
