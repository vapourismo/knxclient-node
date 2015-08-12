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
	ObjectBuilder builder(isolate);

	builder.set("protocol", info.protocol);
	builder.set("port", info.port);

	char address_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &info.address, address_str, INET_ADDRSTRLEN)) {
		builder.set("address", address_str);
	}

	return builder;
}

void knxclient_register_knx_util(Handle<Object>& module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectBuilder builder(isolate, module);

	// Constants
	builder.set("TCP", KNX_PROTO_TCP);
	builder.set("UDP", KNX_PROTO_UDP);
}
