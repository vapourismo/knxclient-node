#include "util.hpp"

extern "C" {
	#include <knxclient/proto/cemi.h>
	#include <knxclient/proto/knxnetip.h>
}

#include <node.h>
#include <node_buffer.h>

using namespace v8;

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

			// L_Data payload
			ObjectBuilder ldata_builder(isolate);
			ldata_builder.set("priority", frame.payload.ldata.control1.priority);
			ldata_builder.set("repeat", frame.payload.ldata.control1.repeat);
			ldata_builder.set("system_broadcast", frame.payload.ldata.control1.system_broadcast);
			ldata_builder.set("request_ack", frame.payload.ldata.control1.request_ack);
			ldata_builder.set("error", frame.payload.ldata.control1.error);
			ldata_builder.set("address_type", frame.payload.ldata.control2.address_type);
			ldata_builder.set("hops", frame.payload.ldata.control2.hops);
			ldata_builder.set("source", frame.payload.ldata.source);
			ldata_builder.set("destination", frame.payload.ldata.destination);
			ldata_builder.set("tpdu", knxclient_tpdu_to_object(isolate, frame.payload.ldata.tpdu));

			// Frame
			ObjectBuilder builder(isolate);
			builder.set("service", frame.service);
			builder.set("payload", ldata_builder);

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

	// Methods
	builder.set("parseCEMI", knxclient_parse_cemi);
	builder.set("parseKNX", knxclient_parse_knx);
}

NODE_MODULE(knxclient, knxclient_init)
