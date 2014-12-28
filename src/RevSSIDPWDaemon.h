/*
 * RevSSIDPWDaemon.h
 *
 *  Created on: Dec 4, 2014
 *      Author: jacky
 */

#ifndef SRC_REVSSIDPWDAEMON_H_
#define SRC_REVSSIDPWDAEMON_H_

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
#include <string.h>
#include "json.h"

#include "NetworkConfig.h"

class RevSSIDPWDaemon
{
public:
	RevSSIDPWDaemon();
	virtual ~RevSSIDPWDaemon();

public:
	bool CreateRevSSIDPWThread();
	NetworkConfig * netconfigptr;

private:
	pthread_t m_RevThreadHande;
	static void *RevThreadFunc(void* lparam);
};

#endif /* SRC_REVSSIDPWDAEMON_H_ */
