#include "util.hpp"

#include <algorithm>
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
