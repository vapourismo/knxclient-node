#ifndef KNXCLIENT_NODE_UTIL_H_
#define KNXCLIENT_NODE_UTIL_H_

extern "C" {
	#include <knxclient/proto/hostinfo.h>
}

#include <node.h>

v8::Local<v8::Object> knxclient_data_to_buffer(v8::Isolate* isolate, const char* data, size_t length);

// TODO: Test 'knxclient_host_info_to_object'
v8::Local<v8::Object> knxclient_host_info_to_object(v8::Isolate* isolate, const knx_host_info& info);

void knxclient_register_knx_util(v8::Handle<v8::Object>& module);

#endif
