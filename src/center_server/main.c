#include <stdio.h>
#include <string.h>
#include  <stdlib.h>
#include "../moduel/net/net_io.h"
#include "../utils/log.h"
#include "./src/cener_config.h"
#include "../moduel/netbus/netbus.h"
#include "../types_service.h"
#include "src/center_services.h"
#include "../database/center_db.h"
int main(int argc, char** argv) {
	init_log();
	//注册center服务模块
	init_server_netbus();
	//初始化连接端使用的协议 
	init_session_manager(TCP_SOCKET_IO, JSON_PROTOCAL);
	
	register_services(SYPTE_CENTER, &CENTER_SERVICE);
	//连接中心服务器
	connect_to_centerdb();
	//启动服务
	//start_server(CENTER_CONF.ip, CENTER_CONF.port, WEB_SOCKET_IO, JSON_PROTOCAL);
	start_server(CENTER_CONF.ip, CENTER_CONF.port, TCP_SOCKET_IO, JSON_PROTOCAL);
	return 0;
}