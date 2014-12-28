/*
 * NetworkConfig.cpp
 *
 *  Created on: 2014-9-7
 *      Author: root
 */

#include "main.h"
#include "NetworkConfig.h"

NetworkConfig::NetworkConfig()
{
	strcpy(wifi_ssid, "TEST");
	strcpy(wifi_key, "12345678");
}

NetworkConfig::~NetworkConfig()
{

}

int NetworkConfig::Init(MainProgram *lp)
{
	m_MainProgram = lp;
	uci_wifi_config_get_from_file();
	return 0;
}

int NetworkConfig::NetworkRestart()
{

	pid_t pid;
	int status;

	if ((pid = fork()) < 0)
	{
		printf(" fork error.\n");
		return -1;
	}
	else if (pid == 0)
	{
		// 执行程序
		//execl("/etc/init.d/network", "network", "restart", NULL); // 执行程序
		execl("/sbin/wifi", "wifi", NULL); // 执行程序
	}
	// parent 进程,等待子进程进程结束
	if ((pid = waitpid(pid, &status, 0)) < 0)
	{
		printf("waitpid error.\n");
		return -1;
	}
	printf("Network Restart Success!\n");
	usleep(50 * 1000);
	return 0;

}
int NetworkConfig::uci_do_set_cmd(struct uci_context *uci_ctx,
		struct uci_ptr *ptr, char *cmd)
{

	if (uci_lookup_ptr(uci_ctx, ptr, cmd, true) != UCI_OK)
	{
		uci_perror(uci_ctx, "uci_lookup_ptr");
		return 1;
	}
	//printf("%s\n", ptr->o->v.string);

	if (uci_set(uci_ctx, ptr) != UCI_OK)
	{
		uci_perror(uci_ctx, "uci_set");
		return -1;
	}
	return 0;
}

int NetworkConfig::uci_do_get_value_cmd(struct uci_context *uci_ctx,
		struct uci_ptr *ptr, char * cmd, char * value)
{

	if (uci_lookup_ptr(uci_ctx, ptr, cmd, true) != UCI_OK)
	{
		uci_perror(uci_ctx, "uci_lookup_ptr");
		return -1;
	}

	if (!(ptr->flags & ptr->UCI_LOOKUP_COMPLETE))
	{
		uci_ctx->err = UCI_ERR_NOTFOUND;
		uci_perror(uci_ctx, "UCI_ERR_NOTFOUND");
		return -1;
	}

	strcpy(value, ptr->o->v.string);
	//printf("%s\n", ptr->o->v.string);

	return 0;
}

int NetworkConfig::uci_wifi_config_set()
{

	struct uci_context *uci_ctx;
	struct uci_ptr ptr;
	char cmd[128];

	uci_ctx = uci_alloc_context();
	if (!uci_ctx)
	{
		fprintf(stderr, "Out of memory\n");
		return -1;
	}

	char *wifi_iface_section = strdup("wireless.@wifi-iface[0]");

	sprintf(cmd, "%s.ssid=%s", wifi_iface_section, wifi_ssid);
	uci_do_set_cmd(uci_ctx, &ptr, cmd);

	sprintf(cmd, "%s.key=%s", wifi_iface_section, wifi_key);
	uci_do_set_cmd(uci_ctx, &ptr, cmd);

	free(wifi_iface_section);

	if (uci_commit(uci_ctx, &ptr.p, false) != UCI_OK)
	{
		uci_perror(uci_ctx, "uci_commit");
	}

	if (ptr.p)
		uci_unload(uci_ctx, ptr.p);

	uci_free_context(uci_ctx);

	//NetworkRestart()使网络重启，新参数生效
	//NetworkRestart();这个函数会使整个工程异常，以后调试

	return 0;
}

int NetworkConfig::uci_wifi_config_get_from_file()
{
	struct uci_context *uci_ctx;
	struct uci_ptr ptr;
	char cmd[128];

	char *wifi_iface_section = strdup("wireless.@wifi-iface[0]");

	uci_ctx = uci_alloc_context();
	if (!uci_ctx)
	{
		fprintf(stderr, "Out of memory\n");
		return -1;
	}

	sprintf(cmd, "%s.ssid", wifi_iface_section);
	uci_do_get_value_cmd(uci_ctx, &ptr, cmd, wifi_ssid);

	sprintf(cmd, "%s.key", wifi_iface_section);
	uci_do_get_value_cmd(uci_ctx, &ptr, cmd, wifi_key);

	free(wifi_iface_section);

	if (ptr.p)
		uci_unload(uci_ctx, ptr.p);

	uci_free_context(uci_ctx);

	printf("wifi_ssid:%s , wifi_key:%s\n", wifi_ssid, wifi_key);

	//进行ssid与password设置，使用uci函数写入配置文件
	uci_wifi_config_set();
	return 0;
}

