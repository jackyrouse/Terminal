/*
 * CommHelper.cpp
 *
 *  Created on: 2014-4-11
 *      Author: root
 */
#include "main.h"
#include "CommHelper.h"

CommHelper::CommHelper()
{
	strcpy(dev, "/dev/");
	Speed = 115200;
	pthread_mutex_init(&WriteReadmutex, NULL);
}

CommHelper::~CommHelper()
{
	tcsetattr(fd, TCSANOW, &oldtio);
	if (fd > 0)
		close(fd);
}

int CommHelper::Close()
{
	return 1;
}

int CommHelper::SendDataToCom(const char * data, int data_len, bool resettag)
{
	int TryTimes = 3;
	int result;
	int tmplen = data_len;
	int n,max_fd,readlen;
	fd_set input;
	max_fd = fd+1;
	struct timeval timeout;
	char* buffer = new char[4096];
//	char* ptr = buffer;
	int ret = 0;
	unsigned char resetretdata1[8] = {0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65};
	unsigned char resetretdata2[6] = {0x55, 0x88, 0xFF, 0x00, 0x00, 0x18};

	printf("\nBegin to execute command!\n");
	while (TryTimes--)
	{
		usleep(50*1000);
		result = write(fd, data, data_len);
		if (result < 0)
		{
			usleep(10 * 1000);
		}
		else
		{
			fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) data[0]);
			for (int i = 1; i < tmplen; i++)
			{
				fprintf(stdout, " %02X", (unsigned char) data[i]);
			}
			fprintf(stdout, "]\n");

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				//读串口返回数据
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000 * 2;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Can not read serial data!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
				}
				steps++;
			}
			buffer[pos] = 0x00;
			fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
			for (int i = 1; i < pos; i++)
			{
				fprintf(stdout, " %02X", (unsigned char) buffer[i]);
			}
			fprintf(stdout, "]\n");
			printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
//			printf("\n%s\n", buffer);
			if(resettag)
			{
				//--判断reset返回值，成功为1，失败为0
				if(!(memcmp(buffer, resetretdata1, 8))||(!memcmp(buffer, resetretdata2, 6)))
				{
					ret = 1;
				}
			}
			if((data[0] == buffer[0])&&(data[1] == buffer[1])&&(data[2] == buffer[2])&&
				(data[0] == buffer[data_len])&&(data[1] == buffer[data_len+1])&&(data[2] == buffer[data_len+2]))
			{
				break;
			}
		}
	}
//	print_hex(data, data_len, "Write to serial port failed");
	printf("End of execute command!\n\n");
	delete [] buffer;
	return ret;
}

int CommHelper::QueryErrInfo(unsigned char * ErrH, unsigned char * ErrL)
{
	*ErrH = 0x00;
	*ErrL = 0x00;
	int result;
	char* realsendbuf = new char[30];
	int tmplen;
	int n,max_fd,readlen;
	fd_set input;
	max_fd = fd+1;
	struct timeval timeout;
	char* buffer = new char[4096];
	char* ptr = buffer;
	//--获取机器状态
	unsigned char machinestatus[2];
	unsigned char machinetime[2];
	memset(realsendbuf, 0, 30);
	realsendbuf[0] = 0x55;
	realsendbuf[1] = 0x07;
	realsendbuf[2] = 0x00;
	realsendbuf[3] = 0x00;
	realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);//0xF2;
	result = write(fd, realsendbuf, 5);
	tmplen = 5;
	if(result > 0)
	{
		if(ReadOUTTag)
		{
			fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
			for (int i = 1; i < tmplen; i++)
			{
				fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
			}
			fprintf(stdout, "]\n");
		}

		int pos = 0;
		int steps = 0;
		tcflush(fd, TCIFLUSH);
		while(true)
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = timeouts * 1000;
			FD_ZERO(&input);
			FD_SET(fd, &input);
			n = select(max_fd, &input, NULL, NULL, &timeout);
			if (n < 0)
				perror("select failed");
			else if (0 == n)
			{
				break;
			}
			else
			{
				ioctl(fd, FIONREAD, &readlen);
				if (!readlen)
				{
					fprintf(stderr, "Failed to get device status in FUNC : QueryErrInfo!\n");
					break;
				}
				readlen = read(fd, &buffer[pos], readlen);
				pos += readlen;
				if(11 == pos)
				{
					break;
				}
			}
			steps++;
		}
		if(ReadOUTTag)
		{
			printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
			for (int i = 1; i < pos; i++)
			{
				fprintf(stdout, " %02X", (unsigned char) buffer[i]);
			}
			fprintf(stdout, "]\n");
		}

		machinestatus[0] = buffer[5+3];
		machinestatus[1] = buffer[5+4];
		char temperrchar = 0x00;
		if(readlen)
		{
			if(machinestatus[0]&0x08)
			{
				//--存在故障，循环查询故障
				unsigned char errtag = 0xff;
				while(errtag)
				{
					memset(realsendbuf, 0, 30);
					realsendbuf[0] = 0x55;
					realsendbuf[1] = 0x06;
					realsendbuf[2] = temperrchar;
					realsendbuf[3] = 0x01;
					realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);//0xF2;
					result = write(fd, realsendbuf, 5);
					tmplen = 5;
					if(result > 0)
					{
						if(ReadOUTTag)
						{
							fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
							for (int i = 1; i < tmplen; i++)
							{
								fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
							}
							fprintf(stdout, "]\n");
						}

						int pos = 0;
						int steps = 0;
						tcflush(fd, TCIFLUSH);
						while(true)
						{
							timeout.tv_sec = 0;
							timeout.tv_usec = timeouts * 1000;
							FD_ZERO(&input);
							FD_SET(fd, &input);
							n = select(max_fd, &input, NULL, NULL, &timeout);
							if (n < 0)
								perror("select failed");
							else if (0 == n)
							{
								break;
							}
							else
							{
								ioctl(fd, FIONREAD, &readlen);
								if (!readlen)
								{
									fprintf(stderr, "Failed to get device err code in FUNC : QueryErrInfo!\n");
									break;
								}
								readlen = read(fd, &buffer[pos], readlen);
								pos += readlen;
								if(11 == pos)
								{
									break;
								}
							}
							steps++;
						}
						if(ReadOUTTag)
						{
							printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
							fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
							for (int i = 1; i < pos; i++)
							{
								fprintf(stdout, " %02X", (unsigned char) buffer[i]);
							}
							fprintf(stdout, "]\n");
						}

						errtag = buffer[7];
						temperrchar = errtag;
						switch(errtag)
						{
						case 0x0C:
							(*ErrL) |= 0x80;
							break;
						case 0x0D:
							(*ErrL) |= 0x40;
							break;
						case 0x15:
							(*ErrL) |= 0x20;
							break;
						case 0x16:
							(*ErrL) |= 0x10;
							break;
						case 0x17:
							(*ErrL) |= 0x08;
							break;
						case 0x18:
							(*ErrL) |= 0x04;
							break;
						case 0x19:
							(*ErrL) |= 0x02;
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
	return 1;
}

int CommHelper::QueryDeviceInfo(bool statustag, bool boundtag, bool time1tag, bool time2tag, bool time3tag, bool otherwaterstag)
{
	//查询设备的所有状态，并将状态保存在DeviceInfo,
	/* DeviceInfo 结构说明
	 * 第00，第01字节含义：状态-H	状态-L
	 * 状态位定义（共2字节）
     * 位	0		1			2			3		4		5		6		7		8		9		10		11	…	14	15
     * 含义	在线		模式			开机			保温		加热		新风		告警		化霜		加氟		测试		未用*	未用*	未用*	未用*	未用*
     *
     * 第02，第03字节含义：出水-H	出水-L
     *
     * 第04，第05字节含义：进水-H	进水-L
     *
     * 第06，第07字节含义：盘管-H	盘管-L
     *
     * 第08，第09字节含义：排气-H	排气-L
     *
     * 第10，第11字节含义：环境-H	环境-L
     *
     * 第12，第13字节含义：小时		分钟
     *
     * 第14，第15字节含义：上限-H	上限-L
     *
     * 第16，第17字节含义：下限-H	下限-L
     *
     * 第18，第19字节含义：起始1-H	起始1-L
     *
     * 第20，第21字节含义：结束1-H	结束1-L
     *
     * 第22，第23字节含义：起始2-H	起始2-L
     *
     * 第24，第25字节含义：结束2-H	结束2-L
     *
     * 第26，第27字节含义：起始3-H	起始3-L
     *
     * 第28，第29字节含义：结束3-H	结束3-L
     *
     * 第30，第31字节含义：代码-H	代码-L	错误代码
	 */

	if(ReadOUTTag)
		printf("Begin to query device information!\n");

	memset(DeviceInfo, 0, 32);

	int result;
	char* realsendbuf = new char[30];
	int tmplen;
	int n,max_fd,readlen;
	fd_set input;
	max_fd = fd+1;
	struct timeval timeout;
	char* buffer = new char[4096];
//	char* ptr = buffer;
	unsigned char machinestatus[2];
	unsigned char machinetime[2];
	unsigned char outwatertempture[2];
	int ISAutoMode;
	unsigned char UpTempture[2];
	unsigned char DownTempture[2];
	unsigned char EcoBeginTime1[2];
	unsigned char EcoEndTime1[2];
	unsigned char EcoBeginTime2[2];
	unsigned char EcoEndTime2[2];
	unsigned char EcoBeginTime3[2];
	unsigned char EcoEndTime3[2];
	unsigned char InwaterTempture[2];
	unsigned char PipeTempture[2];
	unsigned char ExhaustTempture[2];
	unsigned char EnvironmentTempture[2];

	if(statustag)
	{
		//--获取状态，出水，时间
		//--服务器查询机器状态，机器模式，设备时间时需要返回这些参数
		//--cmd : A0	A1	A2

		//--获取机器状态
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x07;
		realsendbuf[2] = 0x00;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);//0xF2;
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
				{
					perror("select failed");
					break;
				}
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device status in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+6) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is55 00 00 AA BB CC DD EE FF 00 A2 03 01 02 00 00 CA 00 00 00 00 C2 AA : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}

			machinestatus[0] = buffer[tmplen+3];
			machinestatus[1] = buffer[tmplen+4];
		}
		//--end of get machine status

		//--获取模式
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x07;
		realsendbuf[2] = 0x01;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device mode in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+6) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			if(0x00 == buffer[tmplen+4])
			{
				ISAutoMode = 1;
			}
			else
			{
				ISAutoMode = 0;
			}
		}
		//--end of get machine mode

		//--获取时间
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x01;
		realsendbuf[2] = 0x02;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+8) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++) {
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			machinetime[0] = buffer[tmplen+3];
			machinetime[1] = buffer[tmplen+4];
		}
		//end of get machine time

		//--获取出水温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x00;
		realsendbuf[2] = 0x06;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device out tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if ((tmplen+8) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++) {
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			outwatertempture[0] = buffer[tmplen+5];
			outwatertempture[1] = buffer[tmplen+6];
		}
		//end of get out water tempture

	//--组织机器状态表示的两个字节 : DeviceInfo[0]，DeviceInfo[1] 字节含义：状态-H	状态-L

	//--机器在线状态
		DeviceInfo[0] = 0x80;

	//--是否为自动模式
		if(!ISAutoMode)
			DeviceInfo[0] |= 0x40;
		else
			DeviceInfo[0] |= 0x00;

	//--判断机器状态
		//--是否为开机
		if((machinestatus[0]&0x40)&&(machinestatus[0]&0x80))
			DeviceInfo[0] |= 0x20;
		//--是否为保温
		if(machinestatus[1]&0x10)
			DeviceInfo[0] |= 0x10;
		//--是否为加热
		if(machinestatus[1]&0x04)
			DeviceInfo[0] |= 0x08;
		//--是否为新风
		if(machinestatus[0]&0x02)
			DeviceInfo[0] |= 0x04;
		//--是否为告警
		if(machinestatus[0]&0x08)
			DeviceInfo[0] |= 0x02;
		//--是否为化霜
		if(machinestatus[1]&0x40)
			DeviceInfo[0] |= 0x01;

		//--是否为加副
		if(machinestatus[0]&0x01)
			DeviceInfo[1] |= 0x80;

		//--是否为测试
		DeviceInfo[1] |= 0x40;

		//--出水温度入DeviceInfo
		DeviceInfo[2] = outwatertempture[0];
		DeviceInfo[3] = outwatertempture[1];

		DeviceInfo[12] = machinetime[0];
		DeviceInfo[13] = machinetime[1];

		if(machinestatus[0]&0x08)
		{
			//告警状态，查询告警代码
		}
	}
	if(boundtag)
	{
		//--获取上下限温度
		//--cmd : A3
		//--获取上限温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x02;
		realsendbuf[2] = 0x00;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device upper tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			UpTempture[0] = buffer[tmplen+5];
			UpTempture[1] = buffer[tmplen+6];
		}
		//--end of get machine up tempture

		//--获取下限温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x02;
		realsendbuf[2] = 0x01;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device floor tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			DownTempture[0] = buffer[tmplen+5];
			DownTempture[1] = buffer[tmplen+6];
		}
		//--end of get machine up tempture

		//--上下限温度入DeviceInfo
		DeviceInfo[14] = UpTempture[0];
		DeviceInfo[15] = UpTempture[1];
		DeviceInfo[16] = DownTempture[0];
		DeviceInfo[17] = DownTempture[1];
	}
	if(time1tag)
	{
		//--获取经济时段1
		//--cmd : A4
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x08;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time1 start in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoBeginTime1[1] = buffer[tmplen+5];
			EcoBeginTime1[0] = buffer[tmplen+6];
		}
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x09;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time1 end in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoEndTime1[1] = buffer[tmplen+5];
			EcoEndTime1[0] = buffer[tmplen+6];
		}
		//--end of get machine Economic time1
		DeviceInfo[18] = EcoBeginTime1[0];
		DeviceInfo[19] = EcoBeginTime1[1];
		DeviceInfo[20] = EcoEndTime1[0];
		DeviceInfo[21] = EcoEndTime1[1];
	}
	if(time2tag)
	{
		//--获取经济时段
		//--cmd : A5
		//--获取经济时段1
		//--cmd : A4
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x0A;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time2 start in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoBeginTime2[1] = buffer[tmplen+5];
			EcoBeginTime2[0] = buffer[tmplen+6];
		}
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x0B;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time2 end in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoEndTime2[1] = buffer[tmplen+5];
			EcoEndTime2[0] = buffer[tmplen+6];
		}
		//--end of get machine Economic time1
		DeviceInfo[22] = EcoBeginTime2[0];
		DeviceInfo[23] = EcoBeginTime2[1];
		DeviceInfo[24] = EcoEndTime2[0];
		DeviceInfo[25] = EcoEndTime2[1];
	}
	if(time3tag)
	{
		//--获取经济时段3
		//--cmd : A6
		//--获取经济时段1
		//--cmd : A4
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x0C;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time3 start in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoBeginTime3[1] = buffer[tmplen+5];
			EcoBeginTime3[0] = buffer[tmplen+6];
		}
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x03;
		realsendbuf[2] = 0x0D;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device time3 end in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+12) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EcoEndTime3[1] = buffer[tmplen+5];
			EcoEndTime3[0] = buffer[tmplen+6];
		}
		//--end of get machine Economic time1
		DeviceInfo[26] = EcoBeginTime3[0];
		DeviceInfo[27] = EcoBeginTime3[1];
		DeviceInfo[28] = EcoEndTime3[0];
		DeviceInfo[29] = EcoEndTime3[1];
	}

	if(otherwaterstag)
	{
		//--获取进水，盘管，环境，排气温度
		//--cmd : 机器自检，服务器不主动获取这些参数
		//--获取进水温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x00;
		realsendbuf[2] = 0x02;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device into tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+8) == pos)
					{
						break;
					}
				}
				steps++;
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			InwaterTempture[0] = buffer[tmplen+5];
			InwaterTempture[1] = buffer[tmplen+6];
		}
		//--end of get in water tempture

		//--获取盘管温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x00;
		realsendbuf[2] = 0x03;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if (0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device pipe tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+8) == pos)
					{
						break;
					}
				}
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			PipeTempture[0] = buffer[tmplen+5];
			PipeTempture[1] = buffer[tmplen+6];
		}
		//--end of get pipe tempture

		//--获取环境温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x00;
		realsendbuf[2] = 0x04;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else if(0 == n)
				{
					break;
				}
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device environment tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+8) == pos)
					{
						break;
					}
				}
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			EnvironmentTempture[0] = buffer[tmplen+5];
			EnvironmentTempture[1] = buffer[tmplen+6];
		}
		//--end of get Environment tempture

		//--获取排气温度
		usleep(60*1000);
		memset(realsendbuf, 0, 30);
		realsendbuf[0] = 0x55;
		realsendbuf[1] = 0x00;
		realsendbuf[2] = 0x05;
		realsendbuf[3] = 0x00;
		realsendbuf[4] = CRC8((unsigned char*)&realsendbuf[1], 3);
		result = write(fd, realsendbuf, 5);
		tmplen = 5;
		if(result > 0)
		{
			if(ReadOUTTag)
			{
				fprintf(stdout, "COM send (Hex):[%02X", (unsigned char) realsendbuf[0]);
				for (int i = 1; i < tmplen; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) realsendbuf[i]);
				}
				fprintf(stdout, "]\n");
			}

			int pos = 0;
			int steps = 0;
			tcflush(fd, TCIFLUSH);
			while(true)
			{
				timeout.tv_sec = 0;
				timeout.tv_usec = timeouts * 1000;
				FD_ZERO(&input);
				FD_SET(fd, &input);
				n = select(max_fd, &input, NULL, NULL, &timeout);
				if (n < 0)
					perror("select failed");
				else
				{
					ioctl(fd, FIONREAD, &readlen);
					if (!readlen)
					{
						fprintf(stderr, "Failed to get device release tempture in FUNC : QueryDeviceInfo!\n");
						break;
					}
					readlen = read(fd, &buffer[pos], readlen);
					pos += readlen;
					if((tmplen+8) == pos)
					{
						break;
					}
				}
			}
			buffer[pos] = 0x00;
	//		printf("com rev :(Str)[%s], reallength is : %d , steps is : %d ,timeouts is : %d\n", buffer, pos, steps, timeouts);

			if(ReadOUTTag)
			{
				fprintf(stdout, "com rev  (Hex):[%02X",(unsigned char) buffer[0]);
				for (int i = 1; i < pos; i++)
				{
					fprintf(stdout, " %02X", (unsigned char) buffer[i]);
				}
				fprintf(stdout, "]\n");
				printf("com rev  (Str):reallength is : %d , steps is : %d ,timeouts is : %d\n", pos, steps, timeouts);
			}
			ExhaustTempture[0] = buffer[tmplen+5];
			ExhaustTempture[1] = buffer[tmplen+6];
		}
		//--end of get exhaust tempture

		DeviceInfo[4] = InwaterTempture[0];
		DeviceInfo[5] = InwaterTempture[1];
		DeviceInfo[6] = PipeTempture[0];
		DeviceInfo[7] = PipeTempture[1];
		DeviceInfo[8] = ExhaustTempture[0];
		DeviceInfo[9] = ExhaustTempture[1];
		DeviceInfo[10] = EnvironmentTempture[0];
		DeviceInfo[11] = EnvironmentTempture[1];
	}

	if(ReadOUTTag)
		printf("End of query device information!\n\n");

	delete [] buffer;
	delete [] realsendbuf;
	return 1;
}

int CommHelper::restructdata(unsigned char * data, int* data_len)
{
	unsigned char * tmpdata = new unsigned char[*data_len*2];
	tmpdata[0] = data[0];
	int t = 1;
	int lenplus = 0;
	for(int i = 1; i < *data_len-1; i++)
	{
		if(0x55 == data[i])
		{
			tmpdata[t] = 0xaa;
			t++;
			tmpdata[t] = 0x5a;
			lenplus++;
		}
		else if(0xaa == data[i])
		{
			tmpdata[t] = 0xaa;
			t++;
			tmpdata[t] = 0xa5;
			lenplus++;
		}
		else
		{
			tmpdata[t] = data[i];
		}
		t++;
	}
	tmpdata[t] = CRC8(&tmpdata[1], t-1);
	memcpy(data, tmpdata, t+1);
	*data_len = t + 1;
	return 0;
}

int CommHelper::SendData(char * str, int len)
{
	//--解析命令并顺利写读串口，向TCPClient中的发送队列加入串口返回值，按照协议规定组织数据
//	int result, TryTimes = 3;
//	char serialno = str[9];
	unsigned char cmd = str[10];
	char datapairs = str[11];
	char* realsendbuf = new char[30];
	memset(realsendbuf, 0, 30);
	int tmplen;
	char* returndata = new char[128];
//	memcpy(SendDataHead, str, 11);

//	int n,max_fd,readlen;
//	fd_set input;
//	max_fd = fd+1;
//	struct timeval timeout;
	char* buffer = new char[128];
//	char* ptr = buffer;

	switch(cmd)
	{
	case 0xA0:
		if(0x01 == datapairs)
		{
			//设置设备开关机状态
			if(0x00 == str[14])
				break;

			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x01;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			SendDataToCom(realsendbuf, tmplen, false);
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA1:
		if(0x01 == datapairs)
		{
			//设置设备模式
			if(0x00 == str[14])
				break;

			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x07;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			SendDataToCom(realsendbuf, tmplen, false);
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA2:
		if(0x01 == datapairs)
		{
			//设置设备时间
			if(0x00 == str[14])
				break;

			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x08;
			realsendbuf[2] = 0x01;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			SendDataToCom(realsendbuf, tmplen, false);
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA3:
		if(0x02 == datapairs)
		{
			//设置上限温度
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x00;
			realsendbuf[3] = str[15];
			realsendbuf[4] = str[16];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);

			if(0x01 == str[17])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			//设置下限温度
			memset(realsendbuf, 0, 30);
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x06;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);
			usleep(60*1000);
			if(0x01 == str[14])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA4:
		if(0x02 == datapairs)
		{
			//设置经济时段1
			//--设置起始时间
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x08;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			if(0x01 == str[14])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			//--设置结束时间
			memset(realsendbuf, 0, 30);
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x09;
			realsendbuf[3] = str[15];
			realsendbuf[4] = str[16];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);
			usleep(60*1000);
			if(0x01 == str[17])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA5:
		if(0x02 == datapairs)
		{
			//设置经济时段2
			//--设置起始时间
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x0A;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			if(0x01 == str[14])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			//--设置结束时间
			memset(realsendbuf, 0, 30);
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x0B;
			realsendbuf[3] = str[15];
			realsendbuf[4] = str[16];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);
			usleep(60*1000);
			if(0x01 == str[17])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xA6:
		if(0x02 == datapairs)
		{
			//设置经济时段3
			//--设置起始时间
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x0C;
			realsendbuf[3] = str[12];
			realsendbuf[4] = str[13];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			usleep(60*1000);
			pthread_mutex_lock(&WriteReadmutex);
			if(0x01 == str[14])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			//--设置结束时间
			memset(realsendbuf, 0, 30);
			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x0D;
			realsendbuf[3] = str[15];
			realsendbuf[4] = str[16];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);
			usleep(60*1000);
			if(0x01 == str[17])
			{
				SendDataToCom(realsendbuf, tmplen, false);
			}
			pthread_mutex_unlock(&WriteReadmutex);
		}
		break;
	case 0xAF:
		if(0x01 == datapairs)
		{
			//恢复出厂设置
			if(0x00 == str[14])
				break;

			realsendbuf[0] = 0x55;
			realsendbuf[1] = 0x05;
			realsendbuf[2] = 0x0D;
			realsendbuf[3] = str[15];
			realsendbuf[4] = str[16];
			realsendbuf[5] = CRC8((unsigned char*)&realsendbuf[1], 4);
			tmplen = 6;
			restructdata((unsigned char*)realsendbuf, &tmplen);

			pthread_mutex_lock(&WriteReadmutex);
			int resetok = SendDataToCom(realsendbuf, tmplen, true);
			pthread_mutex_unlock(&WriteReadmutex);

			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x01;
			returndata[12] = 0x00;

			if(resetok)
				returndata[13] = 0x01;

			returndata[15] = CRC8((unsigned char*)&returndata[1], 14);
			returndata[16] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 17);
		}
		break;

	//--服务器发送至终端的获取代码解析
	case 0xB0:
		if(0x00 == datapairs)
		{
			//查询机器状态
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(true, false, false, false, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x03;
			returndata[12] = DeviceInfo[0];
			returndata[13] = DeviceInfo[1];
			returndata[15] = DeviceInfo[2];
			returndata[16] = DeviceInfo[3];
			returndata[18] = DeviceInfo[12];
			returndata[19] = DeviceInfo[13];
			returndata[21] = CRC8((unsigned char*)&returndata[1], 20);
			returndata[22] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 23);
		}
		break;
	case 0xB1:
		if(0x00 == datapairs)
		{
			//查询机器模式
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(true, false, false, false, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x03;
			returndata[12] = DeviceInfo[0];
			returndata[13] = DeviceInfo[1];
			returndata[15] = DeviceInfo[2];
			returndata[16] = DeviceInfo[3];
			returndata[18] = DeviceInfo[12];
			returndata[19] = DeviceInfo[13];
			returndata[21] = CRC8((unsigned char*)&returndata[1], 20);
			returndata[22] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 23);
		}
		break;
	case 0xB2:
		if(0x00 == datapairs)
		{
			//查询设备时间
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(true, false, false, false, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x03;
			returndata[12] = DeviceInfo[0];
			returndata[13] = DeviceInfo[1];
			returndata[15] = DeviceInfo[2];
			returndata[16] = DeviceInfo[3];
			returndata[18] = DeviceInfo[12];
			returndata[19] = DeviceInfo[13];
			returndata[21] = CRC8((unsigned char*)&returndata[1], 20);
			returndata[22] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 23);
		}
		break;
	case 0xB3:
		if(0x00 == datapairs)
		{
			//查询上下限温度
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(false, true, false, false, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x02;
			returndata[12] = DeviceInfo[16];
			returndata[13] = DeviceInfo[17];
			returndata[15] = DeviceInfo[14];
			returndata[16] = DeviceInfo[15];
			returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
			returndata[19] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		}
		break;
	case 0xB4:
		if(0x00 == datapairs)
		{
			//查询经济时段1
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(false, false, true, false, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x02;
			returndata[12] = DeviceInfo[18];
			returndata[13] = DeviceInfo[19];
			returndata[15] = DeviceInfo[20];
			returndata[16] = DeviceInfo[21];
			returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
			returndata[19] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		}
		break;
	case 0xB5:
		if(0x00 == datapairs)
		{
			//查询经济时段2
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(false, false, false, true, false, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x02;
			returndata[12] = DeviceInfo[22];
			returndata[13] = DeviceInfo[23];
			returndata[15] = DeviceInfo[24];
			returndata[16] = DeviceInfo[25];
			returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
			returndata[19] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		}
		break;
	case 0xB6:
		if(0x00 == datapairs)
		{
			//查询经济时段3
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(false, false, false, false, true, false);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x02;
			returndata[12] = DeviceInfo[26];
			returndata[13] = DeviceInfo[27];
			returndata[15] = DeviceInfo[28];
			returndata[16] = DeviceInfo[29];
			returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
			returndata[19] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		}
		break;
	case 0xB7:
		if(0x00 == datapairs)
		{
			//查询所有温度
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(true, false, false, false, false, true);
			pthread_mutex_unlock(&WriteReadmutex);
			//--分析DeviceInfo数据，之后组织上传服务器的数据，加入发送队列
			memset(returndata, 0, 128);
			memcpy(returndata, str, 11);
			returndata[11] = 0x05;
			returndata[12] = DeviceInfo[2];
			returndata[13] = DeviceInfo[3];
			returndata[15] = DeviceInfo[4];
			returndata[16] = DeviceInfo[5];
			returndata[18] = DeviceInfo[6];
			returndata[19] = DeviceInfo[7];
			returndata[21] = DeviceInfo[8];
			returndata[22] = DeviceInfo[9];
			returndata[24] = DeviceInfo[10];
			returndata[25] = DeviceInfo[11];
			returndata[27] = CRC8((unsigned char*)&returndata[1], 26);
			returndata[28] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 29);
		}
		break;
	case 0xB8:
		if(0x00 == datapairs)
		{
			//查询告警代码
			pthread_mutex_lock(&WriteReadmutex);
			QueryDeviceInfo(true, false, false, false, false, false);
			returndata[11] = 0x01;
			if(DeviceInfo[1]&0x40)
			{
				//--存在故障
				QueryErrInfo((unsigned char*)&returndata[12],(unsigned char*)&returndata[13]);
			}
			else
			{
				//--不存在故障
				returndata[12] = 0x00;
				returndata[13] = 0x00;
			}
			pthread_mutex_unlock(&WriteReadmutex);
			returndata[15] = CRC8((unsigned char*)&returndata[1], 14);
			returndata[16] = 0xAA;
			m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 17);
		}
		break;
	default:
		break;
	}

	delete [] returndata;
//	delete [] buffer;
	delete [] realsendbuf;
	return -1;
}




int CommHelper::init_dev()
{

	struct termios newtio;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		printf("open serial port : %s failed.\n", dev);
		return -1;
	}
	else
		//set to block;
		fcntl(fd, F_SETFL, 0);

	printf("open serial port : %s success.\n", dev);

	tcgetattr(fd, &oldtio); //save current serial port settings

	bzero(&newtio, sizeof(newtio)); // clear struct for new port settings

	int speed_arr[] =
	{ B115200, B38400, B19200, B9600, B4800, B2400, B1200,
	B300, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
	int name_arr[] =
	{ 115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600,
			4800, 2400, 1200, 300, };
	int i;
	for (i = 0; i < (int) sizeof(speed_arr) / (int) sizeof(int); i++)
	{
		if (Speed == name_arr[i])
		{
			cfsetispeed(&newtio, speed_arr[i]);
			cfsetospeed(&newtio, speed_arr[i]);
			i = -1;
			break;
		}
	}
	if (i != -1)
	{
		printf("configure the serial port speed : %d failed.\n", Speed);
		return -1;
	}
	newtio.c_cflag |= CLOCAL | CREAD;
	/*8N1*/
	newtio.c_cflag &= ~CSIZE; /* Mask the character size bits */
	newtio.c_cflag |= CS8; /* Select 8 data bits */
	newtio.c_cflag &= ~PARENB;
	newtio.c_cflag |= CSTOPB; //&= ~CSTOPB;
	newtio.c_cflag &= ~CRTSCTS; //disable hardware flow control;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);/*raw input*/
	newtio.c_oflag &= ~OPOST; /*raw output*/
	tcflush(fd, TCIFLUSH); //clear input buffer
	newtio.c_cc[VTIME] = 100; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 0; /* blocking read until 0 character arrives */
	tcsetattr(fd, TCSANOW, &newtio);

	return 1;
}

//消息
int CommHelper::MsgProcess(const char * data, int data_len)
{

	print_hex(data, data_len, "串口有效数据");

	//need edit code
	//判断是否为要发送的数据，加入发送队列
	m_MainProgram->m_tcpClient.AddToSendQueue(data, data_len);
	//如果为定时查询状态数据，写入文件保存
	//同时更新最近状态保存数组，方便发送
	return 1;
}

//返回的值为已经使用了多少字节
int CommHelper::ExtractData(const char * data, int data_len)
{

	int ret = 0;

	MsgProcess(data, data_len);

	ret = data_len;
	return ret;
}

int CommHelper::AnalyzeData(char info[], int* infolen)
{
	//分析数据并组织上传
	/* DeviceInfo 结构说明
	 * 第00，第01字节含义：状态-H	状态-L
	 * 状态位定义（共2字节）
	 * 位	0		1			2			3		4		5		6		7		8		9		10		11	…	14	15
	 * 含义	在线		模式			开机			保温		加热		新风		告警		化霜		加氟		测试		未用*	未用*	未用*	未用*	未用*
	 *
	 * 第02，第03字节含义：出水-H	出水-L
	 *
	 * 第04，第05字节含义：进水-H	进水-L
	 *
	 * 第06，第07字节含义：盘管-H	盘管-L
	 *
	 * 第08，第09字节含义：排气-H	排气-L
	 *
	 * 第10，第11字节含义：环境-H	环境-L
	 *
	 * 第12，第13字节含义：小时		分钟
	 *
	 * 第14，第15字节含义：上限-H	上限-L
	 *
	 * 第16，第17字节含义：下限-H	下限-L
	 *
	 * 第18，第19字节含义：起始1-H	起始1-L
	 *
	 * 第20，第21字节含义：结束1-H	结束1-L
	 *
	 * 第22，第23字节含义：起始2-H	起始2-L
	 *
	 * 第24，第25字节含义：结束2-H	结束2-L
	 *
	 * 第26，第27字节含义：起始3-H	起始3-L
	 *
	 * 第28，第29字节含义：结束3-H	结束3-L
	 *
	 * 第30，第31字节含义：代码-H	代码-L	错误代码
	 */
	char* returndata = new char[128];
	bool tempturechanged = false;
	bool sectionchanged = false;
	memset(info, 0x00, 64);
	*infolen = 0;
	if((LastDeviceInfo[0] != DeviceInfo[0])||(LastDeviceInfo[1] != DeviceInfo[1]))
	{
		//上传机器状态：命令B0,B1
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB0;
		returndata[11] = 0x03;
		returndata[12] = DeviceInfo[0];
		returndata[13] = DeviceInfo[1];
		returndata[14] = 0x00;
		returndata[21] = CRC8((unsigned char*)&returndata[1], 20);//0xF2;
		returndata[22] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 23);
		LastDeviceInfo[0] = DeviceInfo[0];
		LastDeviceInfo[1] = DeviceInfo[1];
		(*infolen)++;
		info[*infolen] = 0xB0;
		(*infolen)++;
		info[*infolen] = DeviceInfo[0];
		(*infolen)++;
		info[*infolen] = DeviceInfo[1];
	}

	if((LastDeviceInfo[12] != DeviceInfo[12])||(LastDeviceInfo[13] != DeviceInfo[13]))
	{
		//上传机器时间：命令B2
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB2;
		returndata[11] = 0x03;
		returndata[18] = DeviceInfo[12];
		returndata[19] = DeviceInfo[13];
		returndata[20] = 0x00;
		returndata[21] = CRC8((unsigned char*)&returndata[1], 20);
		returndata[22] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 23);
		LastDeviceInfo[12] = DeviceInfo[12];
		LastDeviceInfo[13] = DeviceInfo[13];
	}

	if((LastDeviceInfo[16] != DeviceInfo[16])||(LastDeviceInfo[17] != DeviceInfo[17]))
	{
		//上传下限温度:B3
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB3;
		returndata[11] = 0x02;
		returndata[12] = DeviceInfo[16];
		returndata[13] = DeviceInfo[17];
		returndata[14] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[16] = DeviceInfo[16];
		LastDeviceInfo[17] = DeviceInfo[17];
		tempturechanged = true;
	}
	if((LastDeviceInfo[14] != DeviceInfo[14])||(LastDeviceInfo[15] != DeviceInfo[15]))
	{
		//上传上限温度:B3
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB3;
		returndata[11] = 0x02;
		returndata[15] = DeviceInfo[14];
		returndata[16] = DeviceInfo[15];
		returndata[17] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[14] = DeviceInfo[14];
		LastDeviceInfo[15] = DeviceInfo[15];
		tempturechanged = true;
	}
	if(tempturechanged)
	{
		(*infolen)++;
		info[*infolen] = 0xB3;
		(*infolen)++;
		info[*infolen] = DeviceInfo[16];
		(*infolen)++;
		info[*infolen] = DeviceInfo[17];
		(*infolen)++;
		info[*infolen] = DeviceInfo[14];
		(*infolen)++;
		info[*infolen] = DeviceInfo[15];
		tempturechanged=false;
	}

	if((LastDeviceInfo[18] != DeviceInfo[18])||(LastDeviceInfo[19] != DeviceInfo[19]))
	{
		//上传起始1：B4
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB4;
		returndata[11] = 0x02;
		returndata[12] = DeviceInfo[18];
		returndata[13] = DeviceInfo[19];
		returndata[14] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[18] = DeviceInfo[18];
		LastDeviceInfo[19] = DeviceInfo[19];
		sectionchanged = true;
	}
	if((LastDeviceInfo[20] != DeviceInfo[20])||(LastDeviceInfo[21] != DeviceInfo[21]))
	{
		//上传结束1:B4
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB4;
		returndata[11] = 0x02;
		returndata[15] = DeviceInfo[20];
		returndata[16] = DeviceInfo[21];
		returndata[17] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[20] = DeviceInfo[20];
		LastDeviceInfo[21] = DeviceInfo[21];
		sectionchanged = true;
	}
	if(sectionchanged)
	{
		(*infolen)++;
		info[*infolen] = 0x0B4;
		(*infolen)++;
		info[*infolen] = DeviceInfo[18];
		(*infolen)++;
		info[*infolen] = DeviceInfo[19];
		(*infolen)++;
		info[*infolen] = DeviceInfo[20];
		(*infolen)++;
		info[*infolen] = DeviceInfo[21];
		sectionchanged = false;
	}

	if((LastDeviceInfo[22] != DeviceInfo[22])||(LastDeviceInfo[23] != DeviceInfo[23]))
	{
		//上传起始2：B5
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB5;
		returndata[11] = 0x02;
		returndata[12] = DeviceInfo[22];
		returndata[13] = DeviceInfo[23];
		returndata[14] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[22] = DeviceInfo[22];
		LastDeviceInfo[23] = DeviceInfo[23];
		sectionchanged = true;
	}
	if((LastDeviceInfo[24] != DeviceInfo[24])||(LastDeviceInfo[25] != DeviceInfo[25]))
	{
		//上传结束2:B5
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB5;
		returndata[11] = 0x02;
		returndata[15] = DeviceInfo[24];
		returndata[16] = DeviceInfo[25];
		returndata[17] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[24] = DeviceInfo[24];
		LastDeviceInfo[25] = DeviceInfo[25];
		sectionchanged = true;
	}
	if(sectionchanged)
	{
		(*infolen)++;
		info[*infolen] = 0x0B5;
		(*infolen)++;
		info[*infolen] = DeviceInfo[22];
		(*infolen)++;
		info[*infolen] = DeviceInfo[23];
		(*infolen)++;
		info[*infolen] = DeviceInfo[24];
		(*infolen)++;
		info[*infolen] = DeviceInfo[25];
		sectionchanged = false;
	}

	if((LastDeviceInfo[26] != DeviceInfo[26])||(LastDeviceInfo[27] != DeviceInfo[27]))
	{
		//上传起始3：B6
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB6;
		returndata[11] = 0x02;
		returndata[12] = DeviceInfo[26];
		returndata[13] = DeviceInfo[27];
		returndata[14] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[26] = DeviceInfo[26];
		LastDeviceInfo[27] = DeviceInfo[27];
		sectionchanged = true;
	}
	if((LastDeviceInfo[28] != DeviceInfo[28])||(LastDeviceInfo[29] != DeviceInfo[29]))
	{
		//上传结束3:B6
		memset(returndata, 0x01, 128);
		memcpy(returndata, SendDataHead, 11);
		returndata[10] = 0xB6;
		returndata[11] = 0x02;
		returndata[15] = DeviceInfo[28];
		returndata[16] = DeviceInfo[29];
		returndata[17] = 0x00;
		returndata[18] = CRC8((unsigned char*)&returndata[1], 17);
		returndata[19] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 20);
		LastDeviceInfo[28] = DeviceInfo[28];
		LastDeviceInfo[29] = DeviceInfo[29];
		sectionchanged = true;
	}
	if(sectionchanged)
	{
		(*infolen)++;
		info[*infolen] = 0x0B6;
		(*infolen)++;
		info[*infolen] = DeviceInfo[26];
		(*infolen)++;
		info[*infolen] = DeviceInfo[27];
		(*infolen)++;
		info[*infolen] = DeviceInfo[28];
		(*infolen)++;
		info[*infolen] = DeviceInfo[29];
		sectionchanged = false;
	}

	//查询五个温度的变化
	int lastouttempture = abs((unsigned char)LastDeviceInfo[2]*256+(unsigned char)LastDeviceInfo[3]);
	int lastintempture = abs((unsigned char)LastDeviceInfo[4]*256+(unsigned char)LastDeviceInfo[5]);
	int lastpipetempture = abs((unsigned char)LastDeviceInfo[6]*256+(unsigned char)LastDeviceInfo[7]);
	int lastreleasetempture = abs((unsigned char)LastDeviceInfo[8]*256+(unsigned char)LastDeviceInfo[9]);
	int lastenvtempture = abs((unsigned char)LastDeviceInfo[10]*256+(unsigned char)LastDeviceInfo[11]);

	int outtempture = abs((unsigned char)DeviceInfo[2]*256+(unsigned char)DeviceInfo[3]);
	int intempture = abs((unsigned char)DeviceInfo[4]*256+(unsigned char)DeviceInfo[5]);
	int pipetempture = abs((unsigned char)DeviceInfo[6]*256+(unsigned char)DeviceInfo[7]);
	int releasetempture = abs((unsigned char)DeviceInfo[8]*256+(unsigned char)DeviceInfo[9]);
	int envtempture = abs((unsigned char)DeviceInfo[10]*256+(unsigned char)DeviceInfo[11]);

	int outdifference = abs(lastouttempture - outtempture);
	int indifference = abs(lastintempture - intempture);
	int pipedifference = abs(lastpipetempture - pipetempture);
	int releasedifference = abs(lastreleasetempture - releasetempture);
	int envdifference = abs(lastenvtempture - envtempture);

	bool tempturechangetag = false;
	memset(returndata, 0x01, 128);
	memcpy(returndata, SendDataHead, 11);
	returndata[10] = 0xB7;
	returndata[11] = 0x05;
	if(outdifference > tempturetag)
	{
		returndata[12] = DeviceInfo[2];
		returndata[13] = DeviceInfo[3];
		returndata[14] = 0x00;
		tempturechangetag = true;
		LastDeviceInfo[2] = DeviceInfo[2];
		LastDeviceInfo[3] = DeviceInfo[3];

		(*infolen)++;
		info[*infolen] = 0x0B9;
		(*infolen)++;
		info[*infolen] = DeviceInfo[2];
		(*infolen)++;
		info[*infolen] = DeviceInfo[3];
	}
	if(indifference > tempturetag)
	{
		returndata[15] = DeviceInfo[4];
		returndata[16] = DeviceInfo[5];
		returndata[17] = 0x00;
		tempturechangetag = true;
		LastDeviceInfo[4] = DeviceInfo[4];
		LastDeviceInfo[5] = DeviceInfo[5];
		(*infolen)++;
		info[*infolen] = 0x0BA;
		(*infolen)++;
		info[*infolen] = DeviceInfo[4];
		(*infolen)++;
		info[*infolen] = DeviceInfo[5];
	}
	if(pipedifference > tempturetag)
	{
		returndata[18] = DeviceInfo[6];
		returndata[19] = DeviceInfo[7];
		returndata[20] = 0x00;
		tempturechangetag = true;
		LastDeviceInfo[6] = DeviceInfo[6];
		LastDeviceInfo[7] = DeviceInfo[7];
		(*infolen)++;
		info[*infolen] = 0x0BB;
		(*infolen)++;
		info[*infolen] = DeviceInfo[6];
		(*infolen)++;
		info[*infolen] = DeviceInfo[7];
	}
	if(releasedifference > tempturetag)
	{
		returndata[21] = DeviceInfo[8];
		returndata[22] = DeviceInfo[9];
		returndata[23] = 0x00;
		tempturechangetag = true;
		LastDeviceInfo[8] = DeviceInfo[8];
		LastDeviceInfo[9] = DeviceInfo[9];
		(*infolen)++;
		info[*infolen] = 0x0BC;
		(*infolen)++;
		info[*infolen] = DeviceInfo[8];
		(*infolen)++;
		info[*infolen] = DeviceInfo[9];
	}
	if(envdifference > tempturetag)
	{
		returndata[24] = DeviceInfo[10];
		returndata[25] = DeviceInfo[11];
		returndata[26] = 0x00;
		tempturechangetag = true;
		LastDeviceInfo[10] = DeviceInfo[10];
		LastDeviceInfo[11] = DeviceInfo[11];
		(*infolen)++;
		info[*infolen] = 0x0BD;
		(*infolen)++;
		info[*infolen] = DeviceInfo[10];
		(*infolen)++;
		info[*infolen] = DeviceInfo[11];
	}
	if(tempturechangetag)
	{
		returndata[27] = CRC8((unsigned char*)&returndata[1], 26);
		returndata[28] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 29);
	}

	//查询错误代码
	memset(returndata, 0x01, 128);
	memcpy(returndata, SendDataHead, 11);
	returndata[10] = 0xB8;
	returndata[11] = 0x01;
	if(DeviceInfo[1]&0x02)
	{
		//--存在故障
		QueryErrInfo((unsigned char*)&returndata[12],(unsigned char*)&returndata[13]);
		returndata[14] = 0x00;
		returndata[15] = CRC8((unsigned char*)&returndata[1], 14);
		returndata[16] = 0xAA;
		m_MainProgram->m_tcpClient.AddToSendQueue(returndata, 17);
		DeviceInfo[30] = returndata[12];
		DeviceInfo[31] = returndata[13];
		(*infolen)++;
		info[*infolen] = 0x0B8;
		(*infolen)++;
		info[*infolen] = DeviceInfo[30];
		(*infolen)++;
		info[*infolen] = DeviceInfo[31];
	}

	if(*infolen)
	{
		(*infolen)++;
		info[*infolen] = 0xB2;
		(*infolen)++;
		info[*infolen] = DeviceInfo[12];
		(*infolen)++;
		info[*infolen] = DeviceInfo[13];
		(*infolen)++;
	}
	delete [] returndata;
	return 0;
}

int CommHelper::BuildCurrentDeviceInfo(char info[], int*infolen)
{
	memset(info, 0x00, 64);
	*infolen = 0;
	(*infolen)++;
	info[*infolen] = 0xB0;
	(*infolen)++;
	info[*infolen] = DeviceInfo[0];
	(*infolen)++;
	info[*infolen] = DeviceInfo[1];
	(*infolen)++;
	info[*infolen] = 0xB3;
	(*infolen)++;
	info[*infolen] = DeviceInfo[16];
	(*infolen)++;
	info[*infolen] = DeviceInfo[17];
	(*infolen)++;
	info[*infolen] = DeviceInfo[14];
	(*infolen)++;
	info[*infolen] = DeviceInfo[15];
	(*infolen)++;
	info[*infolen] = 0x0B4;
	(*infolen)++;
	info[*infolen] = DeviceInfo[18];
	(*infolen)++;
	info[*infolen] = DeviceInfo[19];
	(*infolen)++;
	info[*infolen] = DeviceInfo[20];
	(*infolen)++;
	info[*infolen] = DeviceInfo[21];
	(*infolen)++;
	info[*infolen] = 0x0B5;
	(*infolen)++;
	info[*infolen] = DeviceInfo[22];
	(*infolen)++;
	info[*infolen] = DeviceInfo[23];
	(*infolen)++;
	info[*infolen] = DeviceInfo[24];
	(*infolen)++;
	info[*infolen] = DeviceInfo[25];
	(*infolen)++;
	info[*infolen] = 0x0B6;
	(*infolen)++;
	info[*infolen] = DeviceInfo[26];
	(*infolen)++;
	info[*infolen] = DeviceInfo[27];
	(*infolen)++;
	info[*infolen] = DeviceInfo[28];
	(*infolen)++;
	info[*infolen] = DeviceInfo[29];
	(*infolen)++;
	info[*infolen] = 0x0B9;
	(*infolen)++;
	info[*infolen] = DeviceInfo[2];
	(*infolen)++;
	info[*infolen] = DeviceInfo[3];
	(*infolen)++;
	info[*infolen] = 0x0BA;
	(*infolen)++;
	info[*infolen] = DeviceInfo[4];
	(*infolen)++;
	info[*infolen] = DeviceInfo[5];
	(*infolen)++;
	info[*infolen] = 0x0BB;
	(*infolen)++;
	info[*infolen] = DeviceInfo[6];
	(*infolen)++;
	info[*infolen] = DeviceInfo[7];
	(*infolen)++;
	info[*infolen] = 0x0BC;
	(*infolen)++;
	info[*infolen] = DeviceInfo[8];
	(*infolen)++;
	info[*infolen] = DeviceInfo[9];
	(*infolen)++;
	info[*infolen] = 0x0BD;
	(*infolen)++;
	info[*infolen] = DeviceInfo[10];
	(*infolen)++;
	info[*infolen] = DeviceInfo[11];
	(*infolen)++;
	info[*infolen] = 0x0B8;
	(*infolen)++;
	info[*infolen] = DeviceInfo[30];
	(*infolen)++;
	info[*infolen] = DeviceInfo[31];
	(*infolen)++;
	info[*infolen] = 0xB2;
	(*infolen)++;
	info[*infolen] = DeviceInfo[12];
	(*infolen)++;
	info[*infolen] = DeviceInfo[13];
	(*infolen)++;
	info[0] = *infolen;
	return 0;
}

void *CommHelper::CommReadThreadFunc(void * lparam)
{
	//--改成定时(每一秒查询一次，每15秒保存到文件，遇到错误加入发送队列)查询机器状态的线程，与执行远程指令互斥读写串口
	sleep(10);
	int tag = 0;
//	int querytimes = 0;
	CommHelper *pComm;
	//得到CommHelper实例指针
	pComm = (CommHelper*) lparam;
//	char IntervalDeviceInfo[32];
//	int recordfd =  fopen();
	unsigned char lasttime[2];
	lasttime[0] = 0xFF;
	lasttime[1] = 0xFF;
	while(true)
	{
		pthread_mutex_lock(&pComm->WriteReadmutex);
		pComm->QueryDeviceInfo(true,true,true,true,true,true);
		pthread_mutex_unlock(&pComm->WriteReadmutex);

		memcpy(pComm->SendDataHead, pComm->m_MainProgram->m_tcpClient.Headarray, 9);
		pComm->SendDataHead[9] = 0xFF;

		//分析数据并组织上传
		//end of analyze
		pComm->AnalyzeData(pComm->ChangedDeviceInfo, &(pComm->ChangedLen));


		if((0xFF != lasttime[0])&&(0xFF != lasttime[1]))
		{
			if((23==lasttime[0])&&(lasttime[1] >= 59)&&(0 == pComm->DeviceInfo[12])&&(pComm->DeviceInfo[13] < 1))
			{
				//此情况表示隔天，打开下一记录文件，对93取莫
				pComm->statusout.close();
				int tmpstatuspos = atoi(pComm->CurrentRecordPos);
				tmpstatuspos++;
				tmpstatuspos %= 93;
				char tmpstatusfilename[20];
				sprintf(tmpstatusfilename, "/root/status/%02d", tmpstatuspos);
				pComm->statusout.open(tmpstatusfilename, ios::out|ios::binary|ios::trunc);
				sprintf(pComm->CurrentRecordPos, "%02d", tmpstatuspos);
				write_profile_string("StatusPosition", "Position", pComm->CurrentRecordPos, "/root/terminal.ini");
			}
			lasttime[0] = pComm->DeviceInfo[12];
			lasttime[1] = pComm->DeviceInfo[13];
		}
		else
		{
			lasttime[0] = pComm->DeviceInfo[12];
			lasttime[1] = pComm->DeviceInfo[13];
		}
		//每查询10次必须入一次文件
		tag++;
		if(10 == tag)
		{
			tag = 0;
			//每查询10次必须入一次文件
			pComm->BuildCurrentDeviceInfo(pComm->ChangedDeviceInfo, &(pComm->ChangedLen));
			for(int i = 0; i < pComm->ChangedLen; i++)
			{
				if(pComm->statusout.is_open())
				{
					pComm->statusout<<pComm->ChangedDeviceInfo[i];
				}
			}
			pComm->statusout<<endl;
			//将ChangedDeviceInfo入文件
		}
		else
		{
			if(pComm->ChangedLen > 0)
			{
				pComm->ChangedDeviceInfo[0] = pComm->ChangedLen;
				for(int i = 0; i < pComm->ChangedLen; i++)
				{
					if(pComm->statusout.is_open())
					{
						pComm->statusout<<pComm->ChangedDeviceInfo[i];
					}
				}
				pComm->statusout<<endl;
			}
		}

		usleep(1000*1000);
	}
	printf("Interval CommReadThreadFunc exit.\n");
	return 0;
}

int CommHelper::Open(MainProgram *lp, char * device, int speed = 115200)
{
	tempturetag = 2;
	m_MainProgram = lp;
	strcat(dev, device);
	this->Speed = speed;
	if (init_dev() < 0)
		return -1;

	//创建一个线程进行处理，进行定时读取设备状态
	int result = pthread_create(&m_CommReadThreadHandle, NULL, CommReadThreadFunc, (void*) this);

	if (result == -1)
	{
		printf("CommRead work pthread create failed\n");
	}
	else
	{
		//打开设备状态记录文件
		char statusfilename[20] = "/root/status/";
		strcat(statusfilename, CurrentRecordPos);
		statusout.open(statusfilename, ios::out|ios::binary|ios::app);
		if(!statusout.is_open())
			perror("can not open record file!\n");
	}

	return 1;
}

/*
int CommHelper::QueryStatus()
{
	//查询设备状态
	int result;
	unsigned char qsendbuf[10];
	unsigned char qrevbuf[10];
	pthread_mutex_lock(&WriteReadmutex);
	memset(m_MainProgram->m_tcpClient.statusarry, 0xff, 30);
	//查询进水温度
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x00;
	qsendbuf[2] = 0x02;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xce;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[0] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[1] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[0] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[1] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[0] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[1] = 0xff;
		}
	}
	//查询盘管温度
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x00;
	qsendbuf[2] = 0x03;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xdb;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[2] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[3] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[2] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[3] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[2] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[3] = 0xff;
		}
	}
	//查询环境温度
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x00;
	qsendbuf[2] = 0x04;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xb0;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[4] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[5] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[4] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[5] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[4] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[5] = 0xff;
		}
	}
	//查询排气温度
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x00;
	qsendbuf[2] = 0x05;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xa5;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[6] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[7] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[6] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[7] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[6] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[7] = 0xff;
		}
	}
	//查询出水温度
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x00;
	qsendbuf[2] = 0x06;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0x9a;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[8] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[9] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[8] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[9] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[8] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[9] = 0xff;
		}
	}
	//查询设备状态
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x07;
	qsendbuf[2] = 0x00;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xf2;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (6 == read(fd, qrevbuf, 6))
		{
			if (0x07 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[10] = qrevbuf[3];
				m_MainProgram->m_tcpClient.statusarry[11] = qrevbuf[4];

				if (qrevbuf[3] & 0x10)
				{
					for (int i = 0; i < 13; i++)
					{
						memset(qsendbuf, 0, 10);
						memset(qrevbuf, 0, 10);
						qsendbuf[0] = 0x55;
						qsendbuf[1] = 0x06;
						qsendbuf[2] = 0x00;
						qsendbuf[3] = 0x01;
						qsendbuf[4] = 0x9e;
						result = write(fd, qsendbuf, 5);
						if (result)
						{
							if (6 == read(fd, qrevbuf, 6))
							{
								if (0x06 == qrevbuf[1])
								{
									m_MainProgram->m_tcpClient.statusarry[12 + i] =
											qrevbuf[3];
									if (0x00 == qrevbuf[2])
									{
										break;
									}
								}
							}
						}
					}
				}

			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[10] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[11] = 0xff;
			}

		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[10] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[11] = 0xff;
		}
	}
	//读取时间
	memset(qsendbuf, 0, 10);
	memset(qrevbuf, 0, 10);
	qsendbuf[0] = 0x55;
	qsendbuf[1] = 0x01;
	qsendbuf[2] = 0x02;
	qsendbuf[3] = 0x00;
	qsendbuf[4] = 0xa5;
	result = write(fd, qsendbuf, 5);
	if (result)
	{
		if (8 == read(fd, qrevbuf, 8))
		{
			if (0x00 == qrevbuf[1])
			{
				m_MainProgram->m_tcpClient.statusarry[25] = qrevbuf[6];
				m_MainProgram->m_tcpClient.statusarry[26] = qrevbuf[7];
			}
			else
			{
				m_MainProgram->m_tcpClient.statusarry[25] = 0xff;
				m_MainProgram->m_tcpClient.statusarry[26] = 0xff;
			}
		}
		else
		{
			m_MainProgram->m_tcpClient.statusarry[25] = 0xff;
			m_MainProgram->m_tcpClient.statusarry[26] = 0xff;
		}
	}
	//加入到发送队列
	m_MainProgram->m_tcpClient.AddToSendQueue(
			m_MainProgram->m_tcpClient.statusarry, 30);

	//写入状态文件
	pthread_mutex_unlock(&WriteReadmutex);
	return 1;
}
*/
