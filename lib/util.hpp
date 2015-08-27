#ifndef KNXCLIENT_NODE_UTIL_H_
#define KNXCLIENT_NODE_UTIL_H_

#include <algorithm>
#include <string>
#include <node.h>
#include <node_buffer.h>

struct ObjectWrapper: v8::Local<v8::Object> {
	v8::Isolate* isolate;

	inline
	ObjectWrapper(v8::Isolate* isolate):
		v8::Local<v8::Object>(v8::Object::New(isolate)),
		isolate(isolate)
	{}

	inline
	ObjectWrapper(v8::Isolate* isolate, const v8::Handle<v8::Object>& rhs):
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

	inline
	bool has(const char* name) {
		return (*this)->Has(v8::String::NewFromUtf8(isolate, name));
	}

	inline
	v8::Local<v8::Value> get(const char* name) {
		return (*this)->Get(v8::String::NewFromUtf8(isolate, name));
	}

	static
	v8::Local<v8::Object> fromData(v8::Isolate* isolate, const char* data, size_t length);
};

template <typename T>
struct ValueWrapper {
	static_assert(sizeof(T) == -1, "Unknown ValueWrapper specialization");
};

template <>
struct ValueWrapper<int32_t> {
	static
	constexpr const char* description = "Signed Integer";

	static inline
	bool check(const v8::Handle<v8::Value>& handle) {
		return handle->IsInt32();
	}

	static inline
	int32_t unpack(const v8::Handle<v8::Value>& handle) {
		return handle->Int32Value();
	}

	static inline
	v8::Local<v8::Value> unpack(v8::Isolate* isolate, int32_t value) {
		return v8::Integer::New(isolate, value);
	}
};

template <>
struct ValueWrapper<uint32_t> {
	static
	constexpr const char* description = "Unsigned Integer";

	static inline
	bool check(const v8::Handle<v8::Value>& handle) {
		return handle->IsUint32();
	}

	static inline
	uint32_t unpack(const v8::Handle<v8::Value>& handle) {
		return handle->Uint32Value();
	}

	static inline
	v8::Local<v8::Value> unpack(v8::Isolate* isolate, uint32_t value) {
		return v8::Integer::NewFromUnsigned(isolate, value);
	}
};

struct NodeBuffer {
	uint8_t* data;
	size_t length;
};

template <>
struct ValueWrapper<NodeBuffer> {
	static
	constexpr const char* description = "Buffer";

	static
	void freeData(char* ptr, void* ud);

	static inline
	bool check(const v8::Handle<v8::Value>& handle) {
		return node::Buffer::HasInstance(handle);
	}

	static inline
	NodeBuffer unpack(const v8::Handle<v8::Value>& handle) {
		return {(uint8_t*) node::Buffer::Data(handle), node::Buffer::Length(handle)};
	}

	static inline
	v8::Local<v8::Value> unpack(v8::Isolate* isolate, const uint8_t* data, size_t length) {
		char* data_copy = new char[length];
		std::copy(data, data + length, data_copy);
		return node::Buffer::New(isolate, data_copy, length, freeData, nullptr);
	}
};

template <typename T>
struct ParamWrapper {
	static_assert(sizeof(T) == -1, "Unknown ParamWrapper specialization");
};

template <typename R, typename... A>
struct FunctionWrapper {
	template <R (* funptr)(A...)> static
	void wrapped(const v8::FunctionCallbackInfo<v8::Value>& args) {
		v8::Isolate* isolate = args.GetIsolate();

		if (args.Length() < sizeof...(A)) {
			std::string error_string =
				"Expected at least "
				+ std::to_string(sizeof...(A))
				+ " arguments";

			isolate->ThrowException(
				v8::Exception::TypeError(
					v8::String::NewFromUtf8(isolate, error_string.c_str())
				)
			);

			return;
		}
	}
};

#endif
