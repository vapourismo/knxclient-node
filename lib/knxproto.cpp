#include "data.hpp"

#include <node.h>
#include <jawra/objects.hpp>
#include <jawra/functions.hpp>

#include <algorithm>

extern "C" {
	#include <knxproto/router.h>
}

using namespace v8;
using namespace jawra;

static
Handle<Value> copy_buffer(const char* buffer_origin, size_t size) {
	char* buffer = new char[size];
	std::copy(buffer_origin, buffer_origin + size, buffer);

	return knxproto_make_buffer(buffer, size);
}

struct ProcessRoutedShare {
	bool success;
	ObjectWrapper result;
};

static
void process_routed_cemi(
	const knx_router*   router,
	ProcessRoutedShare* data,
	const knx_cemi*     frame
) {
	if (frame->service != KNX_CEMI_LDATA_IND)
		return;

	auto ldata = frame->payload.ldata;

	data->result.set("source",      (uint32_t) ldata.source);
	data->result.set("destination", (uint32_t) ldata.destination);

	switch (ldata.tpdu.tpci) {
		case KNX_TPCI_NUMBERED_DATA:
			data->result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

		case KNX_TPCI_UNNUMBERED_DATA: {
			auto apdu = ldata.tpdu.info.data;

			data->result.set("apci", (uint32_t) apdu.apci);
			data->result.set("data", copy_buffer((char*) apdu.payload, apdu.length));

			break;
		}

		case KNX_TPCI_NUMBERED_CONTROL:
			data->result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

		case KNX_TPCI_UNNUMBERED_CONTROL: {
			data->result.set("control", (uint32_t) ldata.tpdu.info.control);

			break;
		}
	}

	data->success = true;
}

static
Handle<Value> process_routed(Buffer buffer) {
	Isolate* isolate = Isolate::GetCurrent();
	ProcessRoutedShare comm = {false, {isolate}};

	knx_router router = {
		NULL, NULL,
		(knx_router_recv_cb) &process_routed_cemi, &comm
	};

	if (knx_router_process(&router, (const uint8_t*) buffer.data, buffer.length) && comm.success)
		return comm.result;
	else
		return Null(isolate);
}

struct MakeRoutedShare {
	Handle<Value> result;
};

static
void make_routed_message(
	const knx_router* router,
	MakeRoutedShare*  data,
	const uint8_t*    message,
	size_t            message_size
) {
	data->result = copy_buffer((const char*) message, message_size);
}

static
Handle<Value> make_routed(uint32_t source, uint32_t destination, Buffer payload) {
	Isolate* isolate = Isolate::GetCurrent();
	MakeRoutedShare comm = {Null(isolate)};

	knx_router router = {
		(knx_router_send_cb) &make_routed_message, &comm,
		NULL, NULL
	};

	knx_cemi cemi = {
		KNX_CEMI_LDATA_IND,
		0, nullptr
	};

	cemi.payload.ldata = {
		{KNX_LDATA_PRIO_LOW, true, true, true, false},
		{KNX_LDATA_ADDR_GROUP, 7},
		knx_addr(source & 0xFFFF),
		knx_addr(destination & 0xFFFF),
		{KNX_TPCI_UNNUMBERED_DATA, 0}
	};

	cemi.payload.ldata.tpdu.info.data = {
		KNX_APCI_GROUPVALUEWRITE,
		(uint8_t*) payload.data,
		payload.length
	};

	knx_router_send(&router, &cemi);

	return comm.result;
}

static
void knxproto_init(Handle<Object> module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectWrapper module_wrapper(isolate, module);

	module_wrapper.set("processRouted",   JAWRA_WRAP_FUNCTION(process_routed));
	module_wrapper.set("makeRouted",      JAWRA_WRAP_FUNCTION(make_routed));

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
