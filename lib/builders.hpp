#ifndef KNXCLIENT_BUILDERS_H_
#define KNXCLIENT_BUILDERS_H_

#include "util.hpp"

extern "C" {
	#include <knxproto/proto/cemi.h>
	#include <knxproto/proto/knxnetip.h>
}

#include <node.h>

ObjectWrapper knxclient_build_host_info(v8::Isolate* isolate, const knx_host_info& info);

ObjectWrapper knxclient_build_tpdu(v8::Isolate* isolate, const knx_tpdu& tpdu);

ObjectWrapper knxclient_build_ldata(v8::Isolate* isolate, const knx_ldata& ldata);

ObjectWrapper knxclient_build_cemi(v8::Isolate* isolate, const knx_cemi& cemi);

ObjectWrapper knxclient_build_packet(v8::Isolate* isolate, const knx_packet& packet);


#endif
