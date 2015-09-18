#include "data.hpp"

#include <string>
#include <algorithm>

#include <node.h>
#include <node_buffer.h>
#include <arpa/inet.h>

#include <jawra/objects.hpp>
#include <jawra/functions.hpp>
#include <jawra/values.hpp>

extern "C" {
	#include <knxproto/proto/knxnetip.h>
}

using namespace jawra;
using namespace v8;

static
void free_buffer(char* buffer, void*) {
	delete[] buffer;
}

static
Handle<Object> make_buffer(char* buffer, size_t length) {
	return node::Buffer::New(Isolate::GetCurrent(), buffer, length, free_buffer, nullptr);
}

static
Handle<Value> knxproto_make_routed_write(uint32_t src, uint32_t dest, Buffer payload) {
	if (!payload.data || payload.length < 1)
		return Null(Isolate::GetCurrent());

	knx_routing_indication ind = {{KNX_CEMI_LDATA_IND, 0, nullptr}};

	ind.data.payload.ldata = {
		{KNX_LDATA_PRIO_LOW, true, true, true, false},
		{KNX_LDATA_ADDR_GROUP, 7},
		knx_addr(src & 0xFFFF),
		knx_addr(dest & 0xFFFF),
		{KNX_TPCI_UNNUMBERED_DATA, 0}
	};

	ind.data.payload.ldata.tpdu.info.data = {
		KNX_APCI_GROUPVALUEWRITE,
		(uint8_t*) payload.data,
		payload.length
	};

	size_t length = knx_size(KNX_ROUTING_INDICATION, &ind);
	char* buffer = new char[length];
	knx_generate((uint8_t*) buffer, KNX_ROUTING_INDICATION, &ind);

	return make_buffer(buffer, length);
}

static
Handle<Value> knxproto_obj_routed_cemi(const knx_cemi& cemi) {
	if (cemi.service != KNX_CEMI_LDATA_IND)
		return Null(Isolate::GetCurrent());

	const auto& ldata = cemi.payload.ldata;

	if (ldata.tpdu.tpci != KNX_TPCI_UNNUMBERED_DATA && ldata.tpdu.tpci != KNX_TPCI_NUMBERED_DATA)
		return Null(Isolate::GetCurrent());

	if (ldata.tpdu.info.data.apci != KNX_APCI_GROUPVALUEWRITE &&
	    ldata.tpdu.info.data.apci != KNX_APCI_GROUPVALUERESPONSE)
			return Null(Isolate::GetCurrent());


	ObjectWrapper result(Isolate::GetCurrent());
	result.set("source", (uint32_t) ldata.source);
	result.set("destination", (uint32_t) ldata.destination);

	char* payload_copy = new char[ldata.tpdu.info.data.length];
	std::copy(ldata.tpdu.info.data.payload,
	          ldata.tpdu.info.data.payload + ldata.tpdu.info.data.length,
	          payload_copy);

	if (ldata.tpdu.info.data.length > 0)
		payload_copy[0] &= 63;

	Handle<Value> payload_buffer = make_buffer(payload_copy, ldata.tpdu.info.data.length);
	result.set("payload", payload_buffer);

	return result;
}

static
Handle<Value> knxproto_parse_routed_write(Buffer packet) {
	if (!packet.data || packet.length < KNX_HEADER_SIZE)
		return Null(Isolate::GetCurrent());

	knx_packet out;
	if (!knx_parse((const uint8_t*) packet.data, packet.length, &out) ||
	    out.service != KNX_ROUTING_INDICATION)
			return Null(Isolate::GetCurrent());

	return knxproto_obj_routed_cemi(out.payload.routing_ind.data);
}

static
void knxproto_init(Handle<Object> module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectWrapper module_wrapper(isolate, module);

	module_wrapper.set("makeRoutedWrite", JAWRA_WRAP_FUNCTION(knxproto_make_routed_write));
	module_wrapper.set("parseRoutedWrite", JAWRA_WRAP_FUNCTION(knxproto_parse_routed_write));

	module_wrapper.set("parseUnsigned8",  JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned8));
	module_wrapper.set("parseUnsigned16", JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned16));
	module_wrapper.set("parseUnsigned32", JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned32));
	module_wrapper.set("parseSigned8",    JAWRA_WRAP_FUNCTION(knxproto_parse_signed8));
	module_wrapper.set("parseSigned16",   JAWRA_WRAP_FUNCTION(knxproto_parse_signed16));
	module_wrapper.set("parseSigned32",   JAWRA_WRAP_FUNCTION(knxproto_parse_signed32));
	module_wrapper.set("parseFloat16",    JAWRA_WRAP_FUNCTION(knxproto_parse_float16));
	module_wrapper.set("parseFloat32",    JAWRA_WRAP_FUNCTION(knxproto_parse_float32));
	module_wrapper.set("parseBool",       JAWRA_WRAP_FUNCTION(knxproto_parse_bool));
	module_wrapper.set("parseChar",       JAWRA_WRAP_FUNCTION(knxproto_parse_char));
	module_wrapper.set("parseCValue",     JAWRA_WRAP_FUNCTION(knxproto_parse_cvalue));
	module_wrapper.set("parseCStep",      JAWRA_WRAP_FUNCTION(knxproto_parse_cstep));
	module_wrapper.set("parseTimeOfDay",  JAWRA_WRAP_FUNCTION(knxproto_parse_timeofday));
	module_wrapper.set("parseDate",       JAWRA_WRAP_FUNCTION(knxproto_parse_date));

	module_wrapper.set("makeUnsigned8",  JAWRA_WRAP_FUNCTION(knxproto_make_unsigned8));
	module_wrapper.set("makeUnsigned16", JAWRA_WRAP_FUNCTION(knxproto_make_unsigned16));
	module_wrapper.set("makeUnsigned32", JAWRA_WRAP_FUNCTION(knxproto_make_unsigned32));
	module_wrapper.set("makeSigned8",    JAWRA_WRAP_FUNCTION(knxproto_make_signed8));
	module_wrapper.set("makeSigned16",   JAWRA_WRAP_FUNCTION(knxproto_make_signed16));
	module_wrapper.set("makeSigned32",   JAWRA_WRAP_FUNCTION(knxproto_make_signed32));
	module_wrapper.set("makeFloat16",    JAWRA_WRAP_FUNCTION(knxproto_make_float16));
	module_wrapper.set("makeFloat32",    JAWRA_WRAP_FUNCTION(knxproto_make_float32));
	module_wrapper.set("makeBool",       JAWRA_WRAP_FUNCTION(knxproto_make_bool));
	module_wrapper.set("makeChar",       JAWRA_WRAP_FUNCTION(knxproto_make_char));
	module_wrapper.set("makeCValue",     JAWRA_WRAP_FUNCTION(knxproto_make_cvalue));
	module_wrapper.set("makeCStep",      JAWRA_WRAP_FUNCTION(knxproto_make_cstep));
	module_wrapper.set("makeTimeOfDay",  JAWRA_WRAP_FUNCTION(knxproto_make_timeofday));
	module_wrapper.set("makeDate",       JAWRA_WRAP_FUNCTION(knxproto_make_date));
}

NODE_MODULE(knxproto, knxproto_init)
