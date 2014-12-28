/*
 * main.h
 *
 *  Created on: 2014-8-26
 *      Author: root
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>

#include "unit.h"
#include "TCPClient.h"
#include "CommHelper.h"
#include "RevSSIDPWDaemon.h"
#include "inifile.h"

using namespace std;

class MainProgram
{
public:
	NetworkConfig m_NetworkConfig;
	TCPClient m_tcpClient;
	CommHelper m_CommHelper;
	RevSSIDPWDaemon m_RecSSIDPWDaemon;

	int LogPort;
	char LogHost[20];
	int RemotePort;
	char RemoteHost[20];
	int SerSpeed;
	char Serdevice[10];
	char Version[10];
	int VersionH;
	int VersionL;
	char EthName[10];
	char WirelessName[10];
	char GPRSName[10];

	int intervalseconds;
	char CurrentRecordFilename[5];
	int timeouts;

	unsigned char TmpData[20];
	unsigned char MACENC[8];
public:
	MainProgram();
	~MainProgram();
	int init();
	int start();
};

#endif /* MAIN_H_ */
