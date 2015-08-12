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

	// Methods
	builder.set("parseKNX", knxclient_parse_knx);
}
