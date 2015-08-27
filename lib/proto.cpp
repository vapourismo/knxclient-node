#include "util.hpp"
#include "builders.hpp"

extern "C" {
	#include <knxproto/proto/cemi.h>
	#include <knxproto/proto/knxnetip.h>
}

#include <iostream>
#include <node.h>
#include <node_buffer.h>

using namespace v8;

static
void knxclient_parse_packet(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && node::Buffer::HasInstance(args[0])) {
		Handle<Value> value = args[0];

		// Extract buffer details
		const uint8_t* data = (const uint8_t*) node::Buffer::Data(value);
		size_t len = node::Buffer::Length(value);

		// Parse KNXnet frame
		knx_packet frame;
		if (knx_parse(data, len, &frame)) {
			args.GetReturnValue().Set(knxclient_build_packet(isolate, frame));
		} else {
			args.GetReturnValue().SetNull();
		}
	} else {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Argument #0 needs to be a Buffer")
			)
		);
	}
}

void knxclient_parse_apdu(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 1 && node::Buffer::HasInstance(args[0]) && args[1]->IsInt32()) {
		Handle<Value> buf = args[0];

		// Extract buffer details
		const uint8_t* data = (const uint8_t*) node::Buffer::Data(buf);
		size_t len = node::Buffer::Length(buf);

		switch (args[1]->Int32Value()) {
			case KNX_DPT_BOOL: {
				knx_bool value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_BOOL, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_CVALUE: {
				knx_cvalue value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_CVALUE, &value)) {
					ObjectWrapper builder(isolate);

					builder.set("control", value.control);
					builder.set("value", value.value);

					args.GetReturnValue().Set(builder);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_CSTEP: {
				knx_cstep value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_CSTEP, &value)) {
					ObjectWrapper builder(isolate);

					builder.set("control", value.control);
					builder.set("step", value.step);

					args.GetReturnValue().Set(builder);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_CHAR: {
				knx_char value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_CHAR, &value)) {
					args.GetReturnValue().Set(
						String::NewFromOneByte(
							isolate,
							(const uint8_t*) &value,
							String::kNormalString,
							1
						)
					);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_UNSIGNED8: {
				knx_unsigned8 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_UNSIGNED8, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_SIGNED8: {
				knx_signed8 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_SIGNED8, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_UNSIGNED16: {
				knx_unsigned16 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_UNSIGNED16, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_SIGNED16: {
				knx_signed16 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_SIGNED16, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_FLOAT16: {
				knx_float16 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_FLOAT16, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_TIMEOFDAY: {
				knx_timeofday value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_TIMEOFDAY, &value)) {
					ObjectWrapper builder(isolate);

					builder.set("day", value.day);
					builder.set("hour", value.hour);
					builder.set("minute", value.minute);
					builder.set("second", value.second);

					args.GetReturnValue().Set(builder);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_DATE: {
				knx_date value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_DATE, &value)) {
					ObjectWrapper builder(isolate);

					builder.set("day", value.day);
					builder.set("month", value.month);
					builder.set("year", value.year);

					args.GetReturnValue().Set(builder);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_UNSIGNED32: {
				knx_unsigned32 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_UNSIGNED32, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_SIGNED32: {
				knx_signed32 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_SIGNED32, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			case KNX_DPT_FLOAT32: {
				knx_float32 value;
				if (knx_dpt_from_apdu(data, len, KNX_DPT_FLOAT32, &value)) {
					args.GetReturnValue().Set(value);
				} else {
					args.GetReturnValue().SetNull();
				}

				break;
			}

			default:
				args.GetReturnValue().SetNull();
				break;
		}
	} else {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Invalid argument types")
			)
		);
	}
}

static
void knxclient_make_connreq(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	knx_connection_request req = {
		KNX_CONNECTION_REQUEST_TUNNEL,
		KNX_CONNECTION_LAYER_TUNNEL,
		KNX_HOST_INFO_NAT(KNX_PROTO_UDP),
		KNX_HOST_INFO_NAT(KNX_PROTO_UDP)
	};

	uint8_t buffer[KNX_HEADER_SIZE + KNX_CONNECTION_REQUEST_SIZE];
	knx_generate(buffer, KNX_CONNECTION_REQUEST, &req);

	args.GetReturnValue().Set(
		ObjectWrapper::fromData(
			isolate,
			(const char*) buffer,
			KNX_HEADER_SIZE + KNX_CONNECTION_REQUEST_SIZE
		)
	);
}

static
void knxclient_make_dcreq(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && args[0]->IsUint32()) {
		knx_disconnect_request req = {
			uint8_t(args[0]->Uint32Value()),
			0,
			{KNX_PROTO_UDP, 0, 0}
		};

		uint8_t buffer[KNX_HEADER_SIZE + KNX_DISCONNECT_REQUEST_SIZE];
		knx_generate(buffer, KNX_DISCONNECT_REQUEST, &req);

		args.GetReturnValue().Set(
			ObjectWrapper::fromData(
				isolate,
				(const char*) buffer,
				KNX_HEADER_SIZE + KNX_DISCONNECT_REQUEST_SIZE
			)
		);
	} else {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Invalid argument types")
			)
		);
	}
}

static
void knxclient_make_dcres(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && args[0]->IsUint32()) {
		knx_disconnect_response res = {
			uint8_t(args[0]->Uint32Value()),
			0
		};

		uint8_t buffer[KNX_HEADER_SIZE + KNX_DISCONNECT_RESPONSE_SIZE];
		knx_generate(buffer, KNX_DISCONNECT_RESPONSE, &res);

		args.GetReturnValue().Set(
			ObjectWrapper::fromData(
				isolate,
				(const char*) buffer,
				KNX_HEADER_SIZE + KNX_DISCONNECT_RESPONSE_SIZE
			)
		);
	} else {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Invalid argument types")
			)
		);
	}
}

static
void knxclient_make_tunnelres(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 1 && args[0]->IsUint32() && args[1]->IsUint32()) {
		knx_tunnel_response res = {
			uint8_t(args[0]->Uint32Value()),
			uint8_t(args[1]->Uint32Value()),
			0
		};

		uint8_t buffer[KNX_HEADER_SIZE + KNX_TUNNEL_RESPONSE_SIZE];
		knx_generate(buffer, KNX_TUNNEL_RESPONSE, &res);

		args.GetReturnValue().Set(
			ObjectWrapper::fromData(
				isolate,
				(const char*) buffer,
				KNX_HEADER_SIZE + KNX_TUNNEL_RESPONSE_SIZE
			)
		);
	} else {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Invalid argument types")
			)
		);
	}
}

static
void knxclient_init(Handle<Object> module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectWrapper builder(isolate, module);

	// Constants
	builder.set("SearchRequest",              KNX_SEARCH_REQUEST);
	builder.set("SearchResponse",             KNX_SEARCH_RESPONSE);
	builder.set("DescriptionRequest",         KNX_DESCRIPTION_REQUEST);
	builder.set("DescriptionResponse",        KNX_DESCRIPTION_RESPONSE);
	builder.set("ConnectionRequest",          KNX_CONNECTION_REQUEST);
	builder.set("ConnectionResponse",         KNX_CONNECTION_RESPONSE);
	builder.set("ConnectionStateRequest",     KNX_CONNECTION_STATE_REQUEST);
	builder.set("ConnectionStateResponse",    KNX_CONNECTION_STATE_RESPONSE);
	builder.set("DisconnectRequest",          KNX_DISCONNECT_REQUEST);
	builder.set("DisconnectResponse",         KNX_DISCONNECT_RESPONSE);
	builder.set("DeviceConfigurationRequest", KNX_DEVICE_CONFIGURATION_REQUEST);
	builder.set("DeviceConfigurationAck",     KNX_DEVICE_CONFIGURATION_ACK);
	builder.set("TunnelRequest",              KNX_TUNNEL_REQUEST);
	builder.set("TunnelResponse",             KNX_TUNNEL_RESPONSE);
	builder.set("RoutingIndication",          KNX_ROUTING_INDICATION);

	// CEMI constants
	builder.set("LDataReq",                   KNX_CEMI_LDATA_REQ);
	builder.set("LDataInd",                   KNX_CEMI_LDATA_IND);
	builder.set("LDataCon",                   KNX_CEMI_LDATA_CON);

	// LData constants
	builder.set("IndividualAddress",          KNX_LDATA_ADDR_INDIVIDUAL);
	builder.set("GroupAddress",               KNX_LDATA_ADDR_GROUP);

	builder.set("SystemPriority",             KNX_LDATA_PRIO_SYSTEM);
	builder.set("NormalPriority",             KNX_LDATA_PRIO_NORMAL);
	builder.set("UrgentPriority",             KNX_LDATA_PRIO_URGENT);
	builder.set("LowPriority",                KNX_LDATA_PRIO_LOW);

	// TPCI constants
	builder.set("UnnumberedData",             KNX_TPCI_UNNUMBERED_DATA);
	builder.set("NumberedData",               KNX_TPCI_NUMBERED_DATA);
	builder.set("UnnumberedControl",          KNX_TPCI_UNNUMBERED_CONTROL);
	builder.set("NumberedControl",            KNX_TPCI_NUMBERED_CONTROL);
	builder.set("Connected",                  KNX_TPCI_CONTROL_CONNECTED);
	builder.set("Disconnected",               KNX_TPCI_CONTROL_DISCONNECTED);
	builder.set("Ack",                        KNX_TPCI_CONTROL_ACK);
	builder.set("Error",                      KNX_TPCI_CONTROL_ERROR);

	// APCI constants
	builder.set("GroupValueRead",             KNX_APCI_GROUPVALUEREAD);
	builder.set("GroupValueResponse",         KNX_APCI_GROUPVALUERESPONSE);
	builder.set("GroupValueWrite",            KNX_APCI_GROUPVALUEWRITE);
	builder.set("IndividualWrite",            KNX_APCI_INDIVIDUALADDRWRITE);
	builder.set("IndividualRequest",          KNX_APCI_INDIVIDUALADDRREQUEST);
	builder.set("IndividualResponse",         KNX_APCI_INDIVIDUALADDRRESPONSE);
	builder.set("ADCRead",                    KNX_APCI_ADCREAD);
	builder.set("ADCResponse",                KNX_APCI_ADCRESPONSE);
	builder.set("MemoryRead",                 KNX_APCI_MEMORYREAD);
	builder.set("MemoryResponse",             KNX_APCI_MEMORYRESPONSE);
	builder.set("MemoryWrite",                KNX_APCI_MEMORYWRITE);
	builder.set("UserMessage",                KNX_APCI_USERMESSAGE);
	builder.set("MaskVersionRead",            KNX_APCI_MASKVERSIONREAD);
	builder.set("MaskVersionResponse",        KNX_APCI_MASKVERSIONRESPONSE);
	builder.set("Restart",                    KNX_APCI_RESTART);
	builder.set("Escape",                     KNX_APCI_ESCAPE);

	// Data constants
	ObjectWrapper dpt_builder(isolate);

	dpt_builder.set("Bool",                   KNX_DPT_BOOL);
	dpt_builder.set("ControlValue",           KNX_DPT_CVALUE);
	dpt_builder.set("ControlStep",            KNX_DPT_CSTEP);
	dpt_builder.set("Char",                   KNX_DPT_CHAR);
	dpt_builder.set("Unsigned8",              KNX_DPT_UNSIGNED8);
	dpt_builder.set("Signed8",                KNX_DPT_SIGNED8);
	dpt_builder.set("Unsigned16",             KNX_DPT_UNSIGNED16);
	dpt_builder.set("Signed16",               KNX_DPT_SIGNED16);
	dpt_builder.set("Float16",                KNX_DPT_FLOAT16);
	dpt_builder.set("TimeOfDay",              KNX_DPT_TIMEOFDAY);
	dpt_builder.set("Date",                   KNX_DPT_DATE);
	dpt_builder.set("Unsigned32",             KNX_DPT_UNSIGNED32);
	dpt_builder.set("Signed32",               KNX_DPT_SIGNED32);
	dpt_builder.set("Float32",                KNX_DPT_FLOAT32);

	builder.set("dpt", dpt_builder);

	// Methods
	builder.set("parsePacket",                knxclient_parse_packet);
	builder.set("parseAPDU",                  knxclient_parse_apdu);
	builder.set("makeConnectionRequest",      knxclient_make_connreq);
	builder.set("makeDisconnectResponse",     knxclient_make_dcres);
	builder.set("makeDisconnectRequest",      knxclient_make_dcreq);
	builder.set("makeTunnelResponse",         knxclient_make_tunnelres);
}

NODE_MODULE(knxclient_proto, knxclient_init)
