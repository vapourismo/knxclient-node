#include "util.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <node_buffer.h>

using namespace v8;

static
void data_free(char* data, void*) {
	delete[] data;
}

Local<Object> ObjectWrapper::fromData(Isolate* isolate, const char* data, size_t length) {
	char* data_copy = new char[length];
	std::copy(data, data + length, data_copy);
	return node::Buffer::New(isolate, data_copy, length, data_free, nullptr);
}

void ValueWrapper<NodeBuffer>::freeData(char* data, void*) {
	delete[] data;
}
