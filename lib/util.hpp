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

static inline
void knxclient_object_register(
	v8::Isolate* isolate,
	v8::Handle<v8::Object>& module,
	const char* name,
	bool value
) {
	module->Set(v8::String::NewFromUtf8(isolate, name),
	            v8::Boolean::New(isolate, value));
}

static inline
void knxclient_object_register(
	v8::Isolate* isolate,
	v8::Handle<v8::Object>& module,
	const char* name,
	int32_t value
) {
	module->Set(v8::String::NewFromUtf8(isolate, name),
	            v8::Integer::New(isolate, value));
}

#endif
