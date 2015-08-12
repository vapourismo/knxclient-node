#include "util.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <node_buffer.h>

using namespace v8;

static
void data_free(char* data, void*) {
	delete[] data;
}

Local<Object> knxclient_data_to_buffer(Isolate* isolate, const char* data, size_t length) {
	char* data_copy = new char[length];
	std::copy(data, data + length, data_copy);
	return node::Buffer::New(isolate, data_copy, length, data_free, nullptr);
}

Local<Object> knxclient_host_info_to_object(Isolate* isolate, const knx_host_info& info) {
	Local<Object> ret_object = Object::New(isolate);

	ret_object->Set(String::NewFromUtf8(isolate, "protocol"),
	                Integer::New(isolate, info.protocol));

	char address_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &info.address, address_str, INET_ADDRSTRLEN)) {
		ret_object->Set(String::NewFromUtf8(isolate, "address"),
		                String::NewFromUtf8(isolate, address_str));
	}

	ret_object->Set(String::NewFromUtf8(isolate, "port"),
	                Integer::New(isolate, info.port));

	return ret_object;
}

void knxclient_register_knx_util(Handle<Object>& module) {
	Isolate* isolate = Isolate::GetCurrent();

	// Constants
	module->Set(String::NewFromUtf8(isolate, "TCP"),
	            Integer::New(isolate, KNX_PROTO_TCP));
	module->Set(String::NewFromUtf8(isolate, "UDP"),
	            Integer::New(isolate, KNX_PROTO_UDP));
}
