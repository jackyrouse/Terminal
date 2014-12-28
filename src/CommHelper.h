/*
 * CommHelper.h
 *
 *  Created on: 2014-4-11
 *      Author: root
 */

#ifndef COMMHELPER_H_
#define COMMHELPER_H_

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <pthread.h>

#include "unit.h"

using namespace std;

#define ReadOUTTag 0

class MainProgram;

class CommHelper
{

public:
	CommHelper();
	~CommHelper();
	struct termios tio;
	char dev[15];
	int fd;
	ofstream statusout;
	char CurrentRecordPos[5];
	int Speed;
	int timeouts;
	char DeviceInfo[32];
	char LastDeviceInfo[32];
	char ChangedDeviceInfo[64];
	int ChangedLen;
	char SendDataHead[11];
	int tempturetag;
	pthread_t m_CommReadThreadHandle; //通讯线程句柄

	int SendData(char * str, int len);
//	int QueryStatus();
	int QueryDeviceInfo(bool statustag, bool boundtag, bool time1tag, bool time2tag, bool time3tag, bool otherwaterstag);
	int init_dev();
	int Open(MainProgram *lp, char * device, int speed);
	int Close();
	int ExtractData(const char * data, int data_len);
	int MsgProcess(const char * data, int data_len);
	int SendDataToCom(const char * data, int data_len, bool resettag);
	int QueryErrInfo(unsigned char * ErrH, unsigned char * ErrL);
	int restructdata(unsigned char * data, int* data_len);
	int AnalyzeData(char info[], int* infolen);
	int BuildCurrentDeviceInfo(char info[], int* infolen);
	pthread_mutex_t WriteReadmutex; /*初始化互斥锁*/

private:
	MainProgram * m_MainProgram; //引用
	struct termios oldtio;
	static void *CommReadThreadFunc(void * lparam);
};

#endif /* COMMHELPER_H_ */
