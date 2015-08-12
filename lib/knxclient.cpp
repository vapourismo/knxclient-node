#include "knx.hpp"
#include "util.hpp"

extern "C" {
	#include <knxclient/proto/cemi.h>
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
			builder.set("payload", knxclient_data_to_buffer(isolate, (const char*) tpdu.info.data.payload, tpdu.info.data.length));
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
void knxclient_init(Handle<Object> module) {
	Isolate* isolate = Isolate::GetCurrent();
	ObjectBuilder builder(isolate, module);

	// Modules
	knxclient_register_knx_module(module);
	knxclient_register_knx_util(module);

	// Methods
	builder.set("parseCEMI", knxclient_parse_cemi);
}

NODE_MODULE(knxclient, knxclient_init)
