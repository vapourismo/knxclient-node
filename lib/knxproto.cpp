#include "data.hpp"

#include <node.h>
#include <jawra.hpp>

#include <algorithm>

extern "C" {
	#include <knxproto/router.h>
	#include <knxproto/tunnel.h>
}

using namespace v8;
using namespace jawra;

static
Handle<Value> copy_buffer(const char* buffer_origin, size_t size) {
	char* buffer = new char[size];
	std::copy(buffer_origin, buffer_origin + size, buffer);

	return knxproto_make_buffer(buffer, size);
}

struct RouterWrapper {
	Persistent<Function> send;
	Persistent<Function> recv;
	knx_router router;

	static
	void* create(Local<Function> send, Local<Function> recv) {
		Isolate* isolate = Isolate::GetCurrent();

		RouterWrapper* wrapper = new RouterWrapper {
			{isolate, send},
			{isolate, recv}
		};

		knx_router_set_send_handler(&wrapper->router, (knx_router_send_cb) &RouterWrapper::cb_send, wrapper);
		knx_router_set_recv_handler(&wrapper->router, (knx_router_recv_cb) &RouterWrapper::cb_recv, wrapper);

		return wrapper;
	}

	static
	bool process(void* router, Buffer message) {
		if (!router) return false;

		RouterWrapper* wrapper = (RouterWrapper*) router;
		return knx_router_process(&wrapper->router, (const uint8_t*) message.data, message.length);
	}

	static
	void write(void* router, uint32_t src, uint32_t dest, Buffer payload) {
		if (!router) return;

		RouterWrapper* wrapper = (RouterWrapper*) router;

		knx_cemi cemi = {
			KNX_CEMI_LDATA_IND,
			0, nullptr
		};

		cemi.payload.ldata = {
			{KNX_LDATA_PRIO_LOW, true, true, true, false},
			{KNX_LDATA_ADDR_GROUP, 7},
			knx_addr(src & 0xFFFF),
			knx_addr(dest & 0xFFFF),
			{KNX_TPCI_UNNUMBERED_DATA, 0}
		};

		cemi.payload.ldata.tpdu.info.data = {
			KNX_APCI_GROUPVALUEWRITE,
			(uint8_t*) payload.data,
			payload.length
		};

		knx_router_send(&wrapper->router, &cemi);
	}

	static
	void dispose(void* router) {
		if (router) delete (RouterWrapper*) router;
	}

	static
	void cb_send(
		const knx_router* router,
		RouterWrapper*    wrapper,
		const uint8_t*    message,
		size_t            message_size
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		Local<Function> callback = Local<Function>::New(isolate, wrapper->send);

		Local<Value> args[1] = {copy_buffer((const char*) message, message_size)};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}

	static
	void cb_recv(
		const knx_router* router,
		RouterWrapper*    wrapper,
		const knx_cemi*   frame
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		ObjectWrapper result(isolate);

		auto ldata = frame->payload.ldata;

		result.set("service",     (uint32_t) frame->service);
		result.set("source",      (uint32_t) ldata.source);
		result.set("destination", (uint32_t) ldata.destination);

		switch (ldata.tpdu.tpci) {
			case KNX_TPCI_NUMBERED_DATA:
				result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

			case KNX_TPCI_UNNUMBERED_DATA: {
				auto apdu = ldata.tpdu.info.data;

				result.set("apci", (uint32_t) apdu.apci);
				result.set("data", copy_buffer((char*) apdu.payload, apdu.length));

				break;
			}

			case KNX_TPCI_NUMBERED_CONTROL:
				result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

			case KNX_TPCI_UNNUMBERED_CONTROL: {
				result.set("control", (uint32_t) ldata.tpdu.info.control);

				break;
			}
		}

		Local<Function> callback = Local<Function>::New(isolate, wrapper->recv);

		Local<Value> args[1] = {result};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}
};

struct TunnelWrapper {
	Persistent<Function> state_change;
	Persistent<Function> send;
	Persistent<Function> recv;
	Persistent<Function> ack;
	knx_tunnel tunnel;

	static
	void* create(Local<Function> state_change, Local<Function> send, Local<Function> recv, Local<Function> ack) {
		Isolate* isolate = Isolate::GetCurrent();

		TunnelWrapper* wrapper = new TunnelWrapper {
			{isolate, state_change},
			{isolate, send},
			{isolate, recv},
			{isolate, ack},
			{}
		};

		knx_tunnel_set_send_handler(&wrapper->tunnel, (knx_tunnel_send_cb) &TunnelWrapper::cb_send, wrapper);
		knx_tunnel_set_recv_handler(&wrapper->tunnel, (knx_tunnel_recv_cb) &TunnelWrapper::cb_recv, wrapper);
		knx_tunnel_set_state_change_handler(&wrapper->tunnel, (knx_tunnel_state_change_cb) &TunnelWrapper::cb_state_change, wrapper);
		knx_tunnel_set_ack_handler(&wrapper->tunnel, (knx_tunnel_ack_cb) &TunnelWrapper::cb_ack, wrapper);

		return wrapper;
	}

	static
	void dispose(void* tunnel) {
		if (tunnel) delete (TunnelWrapper*) tunnel;
	}

	static
	void connect(void* tunnel) {
		if (!tunnel) return;

		TunnelWrapper* wrapper = (TunnelWrapper*) tunnel;
		knx_tunnel_connect(&wrapper->tunnel);
	}

	static
	bool process(void* tunnel, Buffer message) {
		if (!tunnel) return false;

		TunnelWrapper* wrapper = (TunnelWrapper*) tunnel;
		return knx_tunnel_process(&wrapper->tunnel, (const uint8_t*) message.data, message.length);
	}

	static
	uint32_t write(void* tunnel, uint32_t src, uint32_t dest, Buffer payload, bool request_ack) {
		if (!tunnel) return false;

		TunnelWrapper* wrapper = (TunnelWrapper*) tunnel;

		knx_cemi cemi = {
			KNX_CEMI_LDATA_REQ,
			0, nullptr
		};

		cemi.payload.ldata = {
			{KNX_LDATA_PRIO_LOW, true, true, request_ack, false},
			{KNX_LDATA_ADDR_GROUP, 7},
			knx_addr(src & 0xFFFF),
			knx_addr(dest & 0xFFFF),
			{KNX_TPCI_UNNUMBERED_DATA, 0}
		};

		cemi.payload.ldata.tpdu.info.data = {
			KNX_APCI_GROUPVALUEWRITE,
			(uint8_t*) payload.data,
			payload.length
		};

		return knx_tunnel_send(&wrapper->tunnel, &cemi);
	}

	static
	void disconnect(void* tunnel) {
		if (!tunnel) return;

		TunnelWrapper* wrapper = (TunnelWrapper*) tunnel;
		knx_tunnel_disconnect(&wrapper->tunnel);
	}

	static
	void cb_send(
		const knx_tunnel* tunnel,
		TunnelWrapper*    wrapper,
		const uint8_t*    message,
		size_t            message_size
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		Local<Function> callback = Local<Function>::New(isolate, wrapper->send);

		Local<Value> args[1] = {copy_buffer((const char*) message, message_size)};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}

	static
	void cb_recv(
		const knx_tunnel* tunnel,
		TunnelWrapper*    wrapper,
		const knx_cemi*   frame
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		ObjectWrapper result(isolate);

		auto ldata = frame->payload.ldata;

		result.set("service",     (uint32_t) frame->service);
		result.set("source",      (uint32_t) ldata.source);
		result.set("destination", (uint32_t) ldata.destination);

		result.set("ack",   ldata.control1.request_ack);
		result.set("error", ldata.control1.error);

		switch (ldata.tpdu.tpci) {
			case KNX_TPCI_NUMBERED_DATA:
				result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

			case KNX_TPCI_UNNUMBERED_DATA: {
				auto apdu = ldata.tpdu.info.data;

				result.set("apci", (uint32_t) apdu.apci);
				result.set("data", copy_buffer((char*) apdu.payload, apdu.length));

				break;
			}

			case KNX_TPCI_NUMBERED_CONTROL:
				result.set("sequenceNumber", (uint32_t) ldata.tpdu.seq_number);

			case KNX_TPCI_UNNUMBERED_CONTROL: {
				result.set("control", (uint32_t) ldata.tpdu.info.control);

				break;
			}
		}

		Local<Function> callback = Local<Function>::New(isolate, wrapper->recv);

		Local<Value> args[1] = {result};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}

	static
	void cb_state_change(
		const knx_tunnel* tunnel,
		TunnelWrapper*    wrapper
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		Local<Function> callback = Local<Function>::New(isolate, wrapper->state_change);

		Local<Value> args[1] = {pack<uint32_t>(isolate, tunnel->state)};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}

	static
	void cb_ack(
		const knx_tunnel* tunnel,
		TunnelWrapper*    wrapper,
		uint8_t           seq_number
	) {
		v8::Isolate* isolate = Isolate::GetCurrent();
		Local<Function> callback = Local<Function>::New(isolate, wrapper->ack);

		Local<Value> args[1] = {pack<uint32_t>(isolate, seq_number)};
		callback->Call(isolate->GetCurrentContext(), Null(isolate), 1, args);
	}
};

static
void knxproto_init(Handle<Object> module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectWrapper module_wrapper(isolate, module);

	module_wrapper.set("LDataRequest",      (uint32_t) KNX_CEMI_LDATA_REQ);
	module_wrapper.set("LDataConfirmation", (uint32_t) KNX_CEMI_LDATA_CON);
	module_wrapper.set("LDataIndication",   (uint32_t) KNX_CEMI_LDATA_IND);

	module_wrapper.set("createRouter",  JAWRA_WRAP_FUNCTION(RouterWrapper::create));
	module_wrapper.set("disposeRouter", JAWRA_WRAP_FUNCTION(RouterWrapper::dispose));
	module_wrapper.set("processRouter", JAWRA_WRAP_FUNCTION(RouterWrapper::process));
	module_wrapper.set("writeRouter",   JAWRA_WRAP_FUNCTION(RouterWrapper::write));

	module_wrapper.set("createTunnel",     JAWRA_WRAP_FUNCTION(TunnelWrapper::create));
	module_wrapper.set("disposeTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::dispose));
	module_wrapper.set("connectTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::connect));
	module_wrapper.set("disconnectTunnel", JAWRA_WRAP_FUNCTION(TunnelWrapper::disconnect));
	module_wrapper.set("processTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::process));
	module_wrapper.set("writeTunnel",      JAWRA_WRAP_FUNCTION(TunnelWrapper::write));

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
