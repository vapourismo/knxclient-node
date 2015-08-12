#ifndef KNXCLIENT_NODE_UTIL_H_
#define KNXCLIENT_NODE_UTIL_H_

#include <node.h>

v8::Local<v8::Object> knxclient_data_to_buffer(v8::Isolate* isolate, const char* data, size_t length);

#endif
