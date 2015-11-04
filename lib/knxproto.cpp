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

namespace jawra {
	template <>
	struct ValueWrapper<knx_tpdu> {
		static
		constexpr const char* TypeName = "TPDU";

		static inline
		bool check(v8::Local<v8::Value> value) {
			if (!ValueWrapper<ObjectWrapper>::check(value))
				return false;

			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);

			if (!wrapper.expect<uint32_t>("tpci"))
				return false;

			knx_tpci tpci = (knx_tpci) wrapper.get<uint32_t>("tpci");
			switch (tpci) {
				case KNX_TPCI_NUMBERED_DATA:
					if (!wrapper.expect<uint32_t>("sequenceNumber"))
						return false;

				case KNX_TPCI_UNNUMBERED_DATA:
					return wrapper.expect<uint32_t>("apci") && wrapper.expect<Buffer>("payload");

				case KNX_TPCI_UNNUMBERED_CONTROL:
					if (!wrapper.expect<uint32_t>("sequenceNumber"))
						return false;

				case KNX_TPCI_NUMBERED_CONTROL:
					return wrapper.expect<uint32_t>("control");
			}
		}

		static inline
		knx_tpdu unpack(v8::Local<v8::Value> value) {
			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);
			knx_tpdu tpdu;

			tpdu.tpci = (knx_tpci) wrapper.get<uint32_t>("tpci");
			tpdu.seq_number = 0;

			switch (tpdu.tpci) {
				case KNX_TPCI_NUMBERED_DATA:
					tpdu.seq_number = wrapper.get<uint32_t>("sequenceNumber");

				case KNX_TPCI_UNNUMBERED_DATA: {
					Buffer payload = wrapper.get<Buffer>("payload");

					tpdu.info.data.apci = (knx_apci) wrapper.get<uint32_t>("apci");
					tpdu.info.data.payload = (const uint8_t*) payload.data;
					tpdu.info.data.length = payload.length;

					break;
				}

				case KNX_TPCI_NUMBERED_CONTROL:
					tpdu.seq_number = wrapper.get<uint32_t>("sequenceNumber");

				case KNX_TPCI_UNNUMBERED_CONTROL:
					tpdu.info.control = (knx_tpci_control) wrapper.get<uint32_t>("control");

					break;
			}

			return tpdu;
		}

		static inline
		ObjectWrapper pack(v8::Isolate* isolate, const knx_tpdu& value) {
			ObjectWrapper wrapper(isolate);

			wrapper.set("tpci", (uint32_t) value.tpci);

			switch (value.tpci) {
				case KNX_TPCI_NUMBERED_DATA:
					wrapper.set("sequenceNumber", (uint32_t) value.seq_number);

				case KNX_TPCI_UNNUMBERED_DATA:
					wrapper.set("apci", (uint32_t) value.info.data.apci);
					wrapper.set("payload", copy_buffer((const char*) value.info.data.payload, value.info.data.length));

					break;

				case KNX_TPCI_NUMBERED_CONTROL:
					wrapper.set("sequenceNumber", (uint32_t) value.seq_number);

				case KNX_TPCI_UNNUMBERED_CONTROL:
					wrapper.set("control", (uint32_t) value.info.control);

					break;
			}

			return wrapper;
		}
	};

	template <>
	struct ValueWrapper<knx_ldata> {
		static
		constexpr const char* TypeName = "L_Data";

		static inline
		bool check(v8::Local<v8::Value> value) {
			if (!ValueWrapper<ObjectWrapper>::check(value))
				return false;

			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);
			return wrapper.expect<uint32_t>("destination") && wrapper.expect<knx_tpdu>("tpdu");
		}

		static inline
		knx_ldata unpack(v8::Local<v8::Value> value) {
			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);
			knx_ldata ldata;

			// Control 1
			ldata.control1.priority = wrapper.check<uint32_t>("priority")
			                        ? (knx_ldata_prio) wrapper.get<uint32_t>("priority")
			                        : KNX_LDATA_PRIO_LOW;

			ldata.control1.repeat = wrapper.check<bool>("repeat")
			                      ? wrapper.get<bool>("repeat")
			                      : true;

			ldata.control1.system_broadcast = wrapper.check<bool>("systemBroadcast")
			                                ? wrapper.get<bool>("systemBroadcast")
			                                : true;

			ldata.control1.request_ack = wrapper.check<bool>("requestAck")
			                           ? wrapper.get<bool>("requestAck")
			                           : true;

			ldata.control1.error = wrapper.check<bool>("error")
			                     ? wrapper.get<bool>("error")
			                     : false;


			// Control 2
			ldata.control2.address_type = wrapper.check<uint32_t>("addressType")
			                            ? (knx_ldata_addr_type) wrapper.get<uint32_t>("addressType")
			                            : KNX_LDATA_ADDR_GROUP;

			ldata.control2.hops = wrapper.check<uint32_t>("hops")
			                    ? wrapper.get<uint32_t>("hops")
			                    : 7;

			// Addresses
			ldata.source = wrapper.check<uint32_t>("source")
			             ? (knx_addr) wrapper.get<uint32_t>("source")
			             : 0;

			ldata.destination = (knx_addr) wrapper.get<uint32_t>("destination");

			// Transport unit
			ldata.tpdu = wrapper.get<knx_tpdu>("tpdu");

			return ldata;
		}

		static inline
		ObjectWrapper pack(v8::Isolate* isolate, const knx_ldata& value) {
			ObjectWrapper wrapper(isolate);

			wrapper.set("priority",    (uint32_t) value.control1.priority);
			wrapper.set("repeat",                 value.control1.repeat);
			wrapper.set("systemBroadcast",        value.control1.system_broadcast);
			wrapper.set("requestAck",             value.control1.request_ack);
			wrapper.set("error",                  value.control1.error);
			wrapper.set("addressType", (uint32_t) value.control2.address_type);
			wrapper.set("hops",        (uint32_t) value.control2.hops);

			wrapper.set("source",      (uint32_t) value.source);
			wrapper.set("destination", (uint32_t) value.destination);

			wrapper.set("tpdu",                   value.tpdu);

			return wrapper;
		}
	};

	template <>
	struct ValueWrapper<knx_cemi> {
		static
		constexpr const char* TypeName = "CEMI";

		static inline
		bool check(v8::Local<v8::Value> value) {
			if (!ValueWrapper<ObjectWrapper>::check(value))
				return false;

			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);
			return wrapper.expect<uint32_t>("service") && wrapper.expect<knx_ldata>("payload");
		}

		static inline
		knx_cemi unpack(v8::Local<v8::Value> value) {
			ObjectWrapper wrapper = ValueWrapper<ObjectWrapper>::unpack(value);

			knx_cemi cemi {
				(knx_cemi_service) wrapper.get<uint32_t>("service"),
				0, nullptr
			};

			cemi.payload.ldata = wrapper.get<knx_ldata>("payload");

			return cemi;
		}

		static inline
		ObjectWrapper pack(v8::Isolate* isolate, const knx_cemi& value) {
			ObjectWrapper wrapper(isolate);

			wrapper.set("service", (uint32_t) value.service);
			wrapper.set("payload", ValueWrapper<knx_ldata>::pack(isolate, value.payload.ldata));

			return wrapper;
		}
	};
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
	void m_send(void* router, knx_cemi cemi) {
		if (!router) return;

		RouterWrapper* wrapper = (RouterWrapper*) router;
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
		Local<Function> callback = Local<Function>::New(isolate, wrapper->recv);

		Local<Value> args[1] = {ValueWrapper<knx_cemi>::pack(isolate, *frame)};
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
			{isolate, ack}
		};

		knx_tunnel_init(&wrapper->tunnel);
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
	int32_t m_send(void* tunnel, knx_cemi cemi) {
		if (!tunnel) return -1;

		TunnelWrapper* wrapper = (TunnelWrapper*) tunnel;
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
		Local<Function> callback = Local<Function>::New(isolate, wrapper->recv);

		Local<Value> args[1] = {ValueWrapper<knx_cemi>::pack(isolate, *frame)};
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

	// Constants
	module_wrapper.set("LDataRequest",           (uint32_t) KNX_CEMI_LDATA_REQ);
	module_wrapper.set("LDataConfirmation",      (uint32_t) KNX_CEMI_LDATA_CON);
	module_wrapper.set("LDataIndication",        (uint32_t) KNX_CEMI_LDATA_IND);

	module_wrapper.set("NumberedData",           (uint32_t) KNX_TPCI_NUMBERED_DATA);
	module_wrapper.set("UnnumberedData",         (uint32_t) KNX_TPCI_UNNUMBERED_DATA);
	module_wrapper.set("NumberedControl",        (uint32_t) KNX_TPCI_NUMBERED_CONTROL);
	module_wrapper.set("UnnumberedControl",      (uint32_t) KNX_TPCI_UNNUMBERED_CONTROL);

	module_wrapper.set("GroupValueRead",         (uint32_t) KNX_APCI_GROUPVALUEREAD);
	module_wrapper.set("GroupValueResponse",     (uint32_t) KNX_APCI_GROUPVALUERESPONSE);
	module_wrapper.set("GroupValueWrite",        (uint32_t) KNX_APCI_GROUPVALUEWRITE);
	module_wrapper.set("IndividualAddrWrite",    (uint32_t) KNX_APCI_INDIVIDUALADDRWRITE);
	module_wrapper.set("IndividualAddrRequest",  (uint32_t) KNX_APCI_INDIVIDUALADDRREQUEST);
	module_wrapper.set("IndividualAddrResponse", (uint32_t) KNX_APCI_INDIVIDUALADDRRESPONSE);
	module_wrapper.set("ADCRead",                (uint32_t) KNX_APCI_ADCREAD);
	module_wrapper.set("ADCResponse",            (uint32_t) KNX_APCI_ADCRESPONSE);
	module_wrapper.set("MemoryRead",             (uint32_t) KNX_APCI_MEMORYREAD);
	module_wrapper.set("MemoryResponse",         (uint32_t) KNX_APCI_MEMORYRESPONSE);
	module_wrapper.set("MemoryWrite",            (uint32_t) KNX_APCI_MEMORYWRITE);
	module_wrapper.set("UserMessage",            (uint32_t) KNX_APCI_USERMESSAGE);
	module_wrapper.set("MaskVersionRead",        (uint32_t) KNX_APCI_MASKVERSIONREAD);
	module_wrapper.set("MaskVersionResponse",    (uint32_t) KNX_APCI_MASKVERSIONRESPONSE);
	module_wrapper.set("Restart",                (uint32_t) KNX_APCI_RESTART);
	module_wrapper.set("Escape",                 (uint32_t) KNX_APCI_ESCAPE);

	// Router
	module_wrapper.set("createRouter",  JAWRA_WRAP_FUNCTION(RouterWrapper::create));
	module_wrapper.set("disposeRouter", JAWRA_WRAP_FUNCTION(RouterWrapper::dispose));
	module_wrapper.set("processRouter", JAWRA_WRAP_FUNCTION(RouterWrapper::process));
	module_wrapper.set("sendRouter",    JAWRA_WRAP_FUNCTION(RouterWrapper::m_send));

	// Tunnel
	module_wrapper.set("createTunnel",     JAWRA_WRAP_FUNCTION(TunnelWrapper::create));
	module_wrapper.set("disposeTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::dispose));
	module_wrapper.set("connectTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::connect));
	module_wrapper.set("disconnectTunnel", JAWRA_WRAP_FUNCTION(TunnelWrapper::disconnect));
	module_wrapper.set("processTunnel",    JAWRA_WRAP_FUNCTION(TunnelWrapper::process));
	module_wrapper.set("sendTunnel",       JAWRA_WRAP_FUNCTION(TunnelWrapper::m_send));

	// Parsers
	module_wrapper.set("unpackUnsigned8",  JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned8));
	module_wrapper.set("unpackUnsigned16", JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned16));
	module_wrapper.set("unpackUnsigned32", JAWRA_WRAP_FUNCTION(knxproto_parse_unsigned32));
	module_wrapper.set("unpackSigned8",    JAWRA_WRAP_FUNCTION(knxproto_parse_signed8));
	module_wrapper.set("unpackSigned16",   JAWRA_WRAP_FUNCTION(knxproto_parse_signed16));
	module_wrapper.set("unpackSigned32",   JAWRA_WRAP_FUNCTION(knxproto_parse_signed32));
	module_wrapper.set("unpackFloat16",    JAWRA_WRAP_FUNCTION(knxproto_parse_float16));
	module_wrapper.set("unpackFloat32",    JAWRA_WRAP_FUNCTION(knxproto_parse_float32));
	module_wrapper.set("unpackBool",       JAWRA_WRAP_FUNCTION(knxproto_parse_bool));
	module_wrapper.set("unpackChar",       JAWRA_WRAP_FUNCTION(knxproto_parse_char));
	module_wrapper.set("unpackCValue",     JAWRA_WRAP_FUNCTION(knxproto_parse_cvalue));
	module_wrapper.set("unpackCStep",      JAWRA_WRAP_FUNCTION(knxproto_parse_cstep));
	module_wrapper.set("unpackTimeOfDay",  JAWRA_WRAP_FUNCTION(knxproto_parse_timeofday));
	module_wrapper.set("unpackDate",       JAWRA_WRAP_FUNCTION(knxproto_parse_date));

	// Generators
	module_wrapper.set("packUnsigned8",  JAWRA_WRAP_FUNCTION(knxproto_make_unsigned8));
	module_wrapper.set("packUnsigned16", JAWRA_WRAP_FUNCTION(knxproto_make_unsigned16));
	module_wrapper.set("packUnsigned32", JAWRA_WRAP_FUNCTION(knxproto_make_unsigned32));
	module_wrapper.set("packSigned8",    JAWRA_WRAP_FUNCTION(knxproto_make_signed8));
	module_wrapper.set("packSigned16",   JAWRA_WRAP_FUNCTION(knxproto_make_signed16));
	module_wrapper.set("packSigned32",   JAWRA_WRAP_FUNCTION(knxproto_make_signed32));
	module_wrapper.set("packFloat16",    JAWRA_WRAP_FUNCTION(knxproto_make_float16));
	module_wrapper.set("packFloat32",    JAWRA_WRAP_FUNCTION(knxproto_make_float32));
	module_wrapper.set("packBool",       JAWRA_WRAP_FUNCTION(knxproto_make_bool));
	module_wrapper.set("packChar",       JAWRA_WRAP_FUNCTION(knxproto_make_char));
	module_wrapper.set("packCValue",     JAWRA_WRAP_FUNCTION(knxproto_make_cvalue));
	module_wrapper.set("packCStep",      JAWRA_WRAP_FUNCTION(knxproto_make_cstep));
	module_wrapper.set("packTimeOfDay",  JAWRA_WRAP_FUNCTION(knxproto_make_timeofday));
	module_wrapper.set("packDate",       JAWRA_WRAP_FUNCTION(knxproto_make_date));
}

NODE_MODULE(knxproto, knxproto_init)
