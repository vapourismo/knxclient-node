#include "knx.hpp"
#include "util.hpp"

extern "C" {
	#include <knxclient/proto/knxnetip.h>
}

#include <algorithm>
#include <node_buffer.h>

using namespace v8;

static
Local<Value> knxclient_packet_to_object(Isolate* isolate, const knx_packet& ind) {
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
			break;

		case KNX_ROUTING_INDICATION:
			return knxclient_data_to_buffer(
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
			                knxclient_packet_to_object(isolate, frame));

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

void knxclient_register_knx_module(Handle<Object>& module) {
	Isolate* isolate = Isolate::GetCurrent();

	// Constants
	module->Set(String::NewFromUtf8(isolate, "SearchRequest"),
	            Integer::New(isolate, 0x0201));
	module->Set(String::NewFromUtf8(isolate, "SearchResponse"),
	            Integer::New(isolate, 0x0202));
	module->Set(String::NewFromUtf8(isolate, "DescriptionRequest"),
	            Integer::New(isolate, 0x0203));
	module->Set(String::NewFromUtf8(isolate, "DescriptionResponse"),
	            Integer::New(isolate, 0x0204));
	module->Set(String::NewFromUtf8(isolate, "ConnectionRequest"),
	            Integer::New(isolate, 0x0205));
	module->Set(String::NewFromUtf8(isolate, "ConnectionResponse"),
	            Integer::New(isolate, 0x0206));
	module->Set(String::NewFromUtf8(isolate, "ConnectionStateRequest"),
	            Integer::New(isolate, 0x0207));
	module->Set(String::NewFromUtf8(isolate, "ConnectionStateResponse"),
	            Integer::New(isolate, 0x0208));
	module->Set(String::NewFromUtf8(isolate, "DisconnectRequest"),
	            Integer::New(isolate, 0x0209));
	module->Set(String::NewFromUtf8(isolate, "DisconnectResponse"),
	            Integer::New(isolate, 0x020A));
	module->Set(String::NewFromUtf8(isolate, "DeviceConfigurationRequest"),
	            Integer::New(isolate, 0x0310));
	module->Set(String::NewFromUtf8(isolate, "DeviceConfigurationAck"),
	            Integer::New(isolate, 0x0311));
	module->Set(String::NewFromUtf8(isolate, "TunnelRequest"),
	            Integer::New(isolate, 0x0420));
	module->Set(String::NewFromUtf8(isolate, "TunnelResponse"),
	            Integer::New(isolate, 0x0421));
	module->Set(String::NewFromUtf8(isolate, "RoutingIndication"),
	            Integer::New(isolate, 0x0530));

	// Methods
	module->Set(String::NewFromUtf8(isolate, "parseKNX"),
	            Function::New(isolate, knxclient_parse_knx));
}
