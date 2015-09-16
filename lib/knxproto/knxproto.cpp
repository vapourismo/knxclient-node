#include <string>
#include <algorithm>

#include <node.h>
#include <node_buffer.h>
#include <arpa/inet.h>

#include <jawra/objects.hpp>
#include <jawra/functions.hpp>

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
}

NODE_MODULE(knxproto, knxproto_init)
