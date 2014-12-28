/*
 * NetworkConfig.h
 *
 *  Created on: 2014-9-7
 *      Author: root
 */

#ifndef NETWORKCONFIG_H_
#define NETWORKCONFIG_H_

#include <stdio.h>
#include <string.h>
#include <uci.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "unit.h"

class MainProgram;

class NetworkConfig
{
private:
public:
	NetworkConfig();
	~NetworkConfig();
	int Init(MainProgram *lp);
	int NetworkRestart();
	int uci_do_set_cmd(struct uci_context *uci_ctx, struct uci_ptr *ptr,
			char *cmd);
	int uci_do_get_value_cmd(struct uci_context *uci_ctx, struct uci_ptr *ptr,
			char * cmd, char * value);
	int uci_wifi_config_set();
	int uci_wifi_config_get_from_file();

public:
	MainProgram * m_MainProgram; //引用
	char wifi_ssid[33];
	char wifi_key[33];

};

#endif /* NETWORKCONFIG_H_ */
