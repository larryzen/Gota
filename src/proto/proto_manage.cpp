#include "proto_manage.h"
#include "../moduel/netbus/recv_msg.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/net/net_uv.h"
#include "../utils/mem_manger.h"

using namespace google::protobuf;

std::unordered_map<int, std::string> proroManager::_cmd_map;

void proroManager::register_cmd(int cmd_id, char* cmd) {

}

void proroManager::register_cmd(char** cmd_array, int cmd_count) {
	if (cmd_count <=0) {
		return;
	}

	for (int i = 0; i < cmd_count;++i) {
		_cmd_map[i]= std::string(cmd_array[i]);
	}
}

const std::string proroManager::get_cmmand_protoname(int cmd) {
	std::unordered_map<int, std::string>::iterator it = _cmd_map.find(cmd);
	if (it!= _cmd_map.end()) {
		return it->second;
	}

	return "";
}

//使用pb的反射机制创建对应的message对象 14
Message* proroManager::create_message_by_name(const std::string& type_name) {
	if (type_name.empty()) {
		return NULL;
	}
	//获取descriptor，每一个message都有一个定义的 descriptor
	//descriptor定义了message的元信息
	const Descriptor* descriptor  = DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if (descriptor == NULL) {
		return NULL;
	}

	const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
	if (prototype==NULL) {
		return NULL;
	}
	Message* message = prototype->New();
	return message;
}
//解析到完成二进制包调用
bool proroManager::decode_cmd_msg(unsigned char* pkg, int pkg_len, struct recv_msg** out_msg) {
	if (pkg==NULL || pkg_len<=0) {
		return false;
	}

	*out_msg = NULL;
	if (pkg_len < BIN_HEAD_LEN) {
		return false;
	}

	//先解码包头
	//recv_msg* msg = (recv_msg*)malloc(sizeof(recv_msg));
	recv_msg* msg = (recv_msg*)memory_mgr::get_instance().alloc_memory(sizeof(recv_msg));
	if (msg==NULL) {
		return false;
	}
	memset(msg,0,sizeof(recv_msg));
	*out_msg = msg;
	msg->stype = pkg[0] + (pkg[1] << 8);
	msg->ctype = pkg[2] + (pkg[3] << 8);
	msg->utag = pkg[4] + (pkg[5] << 8) + (pkg[6] << 16) + (pkg[7] << 24);

	if (pkg_len==BIN_HEAD_LEN) {
		
		return true;
	}
	const std::string type_name = proroManager::get_cmmand_protoname(msg->ctype);
	if (type_name.empty()) {
		free(msg);
		msg = NULL;
		return false;
	}

	Message* pb_msg = create_message_by_name(type_name);
	if (pb_msg ==NULL) {
		free(msg);
		msg = NULL;
		return false;
	}

	if (false==pb_msg->ParseFromArray(pkg+ BIN_HEAD_LEN, pkg_len- BIN_HEAD_LEN)) {
		free(msg);
		msg = NULL;
		delete pb_msg;
		return false;
	}

	msg->body = pb_msg;
	*out_msg = msg;
	return true;
}

unsigned char* proroManager::encode_cmd_msg(recv_msg* msg, int * out_len) {
	if (msg==NULL) {
		*out_len = 0;
		return NULL;
	}

	unsigned char* package = NULL;
	int total_len = 0;
	if (get_proto_type() == BIN_PROTOCAL) {
		Message* pb_msg = (Message*)msg->body;
		total_len = BIN_HEAD_LEN + pb_msg->ByteSize();
		package = (unsigned char*)malloc(total_len);
		if (package==NULL) {
			*out_len = 0;
			return NULL;
		}
		if (false==pb_msg->SerializePartialToArray(package+ BIN_HEAD_LEN,pb_msg->ByteSize())) {
			*out_len = 0;
			return NULL;
		}

		*out_len = BIN_HEAD_LEN + pb_msg->ByteSize();
	}
	else{
		//json
	}

	//消息头
	package[0] = (msg->stype & 0x000000ff);
	package[1] = ((msg->stype & 0x0000ff00) >> 8);
	package[2] = (msg->ctype & 0x000000ff);
	package[3] = ((msg->ctype & 0x0000ff00) >> 8);
	memcpy(package + 4, &msg->utag, sizeof(msg->utag));
	
	return package;
}

void proroManager::msg_free(recv_msg* msg) {
	if (msg==NULL) {
		return;
	}

	if (get_proto_type() == BIN_PROTOCAL) {
		if (msg->body != NULL) {
			Message* pb_msg = (Message*)msg->body;
			delete pb_msg;
		}
	}
	/*else if (get_proto_type() == JSON_PROTOCAL) {
		free(msg->body);
	}*/

	free(msg);
}