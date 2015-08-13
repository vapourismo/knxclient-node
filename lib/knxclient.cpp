#include "util.hpp"

extern "C" {
	#include <knxclient/proto/cemi.h>
	#include <knxclient/proto/knxnetip.h>
}

#include <node.h>
#include <node_buffer.h>

using namespace v8;

static
Local<Value> knxclient_knx_to_object(Isolate* isolate, const knx_packet& ind) {
	switch (ind.service) {
		case KNX_SEARCH_REQUEST:
		case KNX_SEARCH_RESPONSE:
		case KNX_DESCRIPTION_REQUEST:
		case KNX_DESCRIPTION_RESPONSE:
		case KNX_CONNECTION_REQUEST:
		case KNX_CONNECTION_RESPONSE:
		case KNX_CONNECTION_STATE_REQUEST:
		case KNX_CONNECTION_STATE_RESPONSE:
		case KNX_DISCONNECT_REQUEST:
		case KNX_DISCONNECT_RESPONSE:
		case KNX_DEVICE_CONFIGURATION_REQUEST:
		case KNX_DEVICE_CONFIGURATION_ACK:
		case KNX_TUNNEL_REQUEST:
		case KNX_TUNNEL_RESPONSE:
			// TODO: Implement me
			break;

		case KNX_ROUTING_INDICATION:
			return ObjectBuilder::fromData(
				isolate,
				(const char*) ind.payload.routing_ind.data,
				ind.payload.routing_ind.size
			);
	}

	return Object::New(isolate);
}

static
void knxclient_parse_knx(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && node::Buffer::HasInstance(args[0])) {
		Handle<Value> value = args[0];

		// Extract buffer details
		const uint8_t* data = (const uint8_t*) node::Buffer::Data(value);
		size_t len = node::Buffer::Length(value);

		// Parse KNXnet frame
		knx_packet frame;
		if (knx_parse(data, len, &frame)) {
			Local<Object> ret_object = Object::New(isolate);

			ret_object->Set(String::NewFromUtf8(isolate, "service"),
			                Integer::New(isolate, frame.service));
			ret_object->Set(String::NewFromUtf8(isolate, "payload"),
			                knxclient_knx_to_object(isolate, frame));

			args.GetReturnValue().Set(ret_object);
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

static
Local<Object> knxclient_tpdu_to_object(Isolate* isolate, const knx_tpdu& tpdu) {
	ObjectBuilder builder(isolate);

	builder.set("tpci", tpdu.tpci);
	builder.set("seq_number", tpdu.seq_number);

	switch (tpdu.tpci) {
		case KNX_TPCI_UNNUMBERED_DATA:
		case KNX_TPCI_NUMBERED_DATA:
			builder.set("apci", tpdu.info.data.apci);
			builder.set("payload", ObjectBuilder::fromData(isolate, (const char*) tpdu.info.data.payload, tpdu.info.data.length));
			break;

		case KNX_TPCI_UNNUMBERED_CONTROL:
		case KNX_TPCI_NUMBERED_CONTROL:
			builder.set("control", tpdu.info.control);
			break;
	}

	return builder;
}

static
Local<Object> knxclient_ldata_to_object(Isolate* isolate, const knx_ldata& ldata) {
	ObjectBuilder builder(isolate);

	builder.set("priority",         ldata.control1.priority);
	builder.set("repeat",           ldata.control1.repeat);
	builder.set("system_broadcast", ldata.control1.system_broadcast);
	builder.set("request_ack",      ldata.control1.request_ack);
	builder.set("error",            ldata.control1.error);
	builder.set("address_type",     ldata.control2.address_type);
	builder.set("hops",             ldata.control2.hops);
	builder.set("source",           ldata.source);
	builder.set("destination",      ldata.destination);
	builder.set("tpdu",             knxclient_tpdu_to_object(isolate, ldata.tpdu));

	return builder;
}

static
void knxclient_parse_cemi(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && node::Buffer::HasInstance(args[0])) {
		Handle<Value> value = args[0];

		// Extract buffer details
		const uint8_t* data = (const uint8_t*) node::Buffer::Data(value);
		size_t len = node::Buffer::Length(value);

		// Parse CEMI
		knx_cemi_frame frame;
		if (knx_cemi_parse(data, len, &frame)) {
			// Frame
			ObjectBuilder builder(isolate);
			builder.set("service", frame.service);
			builder.set("payload", knxclient_ldata_to_object(isolate, frame.payload.ldata));

			args.GetReturnValue().Set(builder);
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
					ObjectBuilder builder(isolate);

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
					ObjectBuilder builder(isolate);

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
					ObjectBuilder builder(isolate);

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
					ObjectBuilder builder(isolate);

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
void knxclient_extract_ldata(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	if (args.Length() > 0 && node::Buffer::HasInstance(args[0])) {
		Handle<Value> buf = args[0];

		// Extract buffer details
		const uint8_t* data = (const uint8_t*) node::Buffer::Data(buf);
		size_t len = node::Buffer::Length(buf);

		knx_packet frame;

		if (knx_parse(data, len, &frame)) {
			bool worked = false;
			knx_cemi_frame cemi;

			if (frame.service == KNX_ROUTING_INDICATION) {
				worked = knx_cemi_parse((const uint8_t*) frame.payload.routing_ind.data,
				                        frame.payload.routing_ind.size,
				                        &cemi);
			} else if (frame.service == KNX_TUNNEL_REQUEST) {
				worked = knx_cemi_parse((const uint8_t*) frame.payload.tunnel_req.data,
				                        frame.payload.tunnel_req.size,
				                        &cemi);
			}

			if (worked && (cemi.service == KNX_CEMI_LDATA_REQ || cemi.service == KNX_CEMI_LDATA_IND)) {
				args.GetReturnValue().Set(knxclient_ldata_to_object(isolate, cemi.payload.ldata));
				return;
			}
		}

		args.GetReturnValue().SetNull();
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
	ObjectBuilder builder(isolate, module);

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

	builder.set("Bool",                       KNX_DPT_BOOL);
	builder.set("ControlValue",               KNX_DPT_CVALUE);
	builder.set("ControlStep",                KNX_DPT_CSTEP);
	builder.set("Char",                       KNX_DPT_CHAR);
	builder.set("Unsigned8",                  KNX_DPT_UNSIGNED8);
	builder.set("Signed8",                    KNX_DPT_SIGNED8);
	builder.set("Unsigned16",                 KNX_DPT_UNSIGNED16);
	builder.set("Signed16",                   KNX_DPT_SIGNED16);
	builder.set("Float16",                    KNX_DPT_FLOAT16);
	builder.set("TimeOfDay",                  KNX_DPT_TIMEOFDAY);
	builder.set("Date",                       KNX_DPT_DATE);
	builder.set("Unsigned32",                 KNX_DPT_UNSIGNED32);
	builder.set("Signed32",                   KNX_DPT_SIGNED32);
	builder.set("Float32",                    KNX_DPT_FLOAT32);

	// Methods
	builder.set("parseKNX",                   knxclient_parse_knx);
	builder.set("parseCEMI",                  knxclient_parse_cemi);
	builder.set("parseAPDU",                  knxclient_parse_apdu);
	builder.set("extractLData",               knxclient_extract_ldata);
}

NODE_MODULE(knxclient, knxclient_init)
