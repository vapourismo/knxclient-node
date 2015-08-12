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

struct ObjectBuilder: v8::Local<v8::Object> {
	v8::Isolate* isolate;

	inline
	ObjectBuilder(v8::Isolate* isolate):
		v8::Local<v8::Object>(v8::Object::New(isolate)),
		isolate(isolate)
	{}

	inline
	ObjectBuilder(v8::Isolate* isolate, const v8::Handle<v8::Object>& rhs):
		v8::Local<v8::Object>(rhs),
		isolate(isolate)
	{}

	inline
	void set(const char* name, int32_t value) {
		set(name, v8::Integer::New(isolate, value));
	}

	inline
	void set(const char* name, bool value) {
		set(name, v8::Boolean::New(isolate, value));
	}

	inline
	void set(const char* name, const char* value) {
		set(name, v8::String::NewFromUtf8(isolate, value));
	}

	inline
	void set(const char* name, v8::FunctionCallback value) {
		set(name, v8::Function::New(isolate, value));
	}

	template <typename T> inline
	void set(const char* name, const Handle<T>& value) {
		(*this)->Set(v8::String::NewFromUtf8(isolate, name), value);
	}
};

#endif
