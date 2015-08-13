#ifndef KNXCLIENT_NODE_UTIL_H_
#define KNXCLIENT_NODE_UTIL_H_

#include <node.h>

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
	void set(const char* name, uint32_t value) {
		set(name, v8::Integer::NewFromUnsigned(isolate, value));
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

	static
	v8::Local<v8::Object> fromData(v8::Isolate* isolate, const char* data, size_t length);
};

#endif
