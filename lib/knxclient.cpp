#include "router.hpp"
#include <node.h>

using namespace v8;

void knxclient_init(Handle<Object> module) {
	knxclient_init_router(module);
}

NODE_MODULE(knxclient, knxclient_init)
