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
	Local<Object> ret_object = Object::New(isolate);



	return ret_object;
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
			Local<Object> frame_object = Object::New(isolate);

			// Header
			frame_object->Set(String::NewFromUtf8(isolate, "service"),
			                  Integer::New(isolate, frame.service));

			// Payload
			Local<Object> ldata_object = Object::New(isolate);

			knxclient_object_register(isolate, ldata_object, "priority", frame.payload.ldata.control1.priority);
			knxclient_object_register(isolate, ldata_object, "repeat", frame.payload.ldata.control1.repeat);
			knxclient_object_register(isolate, ldata_object, "system_broadcast", frame.payload.ldata.control1.system_broadcast);
			knxclient_object_register(isolate, ldata_object, "request_ack", frame.payload.ldata.control1.request_ack);
			knxclient_object_register(isolate, ldata_object, "error", frame.payload.ldata.control1.error);

			knxclient_object_register(isolate, ldata_object, "address_type", frame.payload.ldata.control2.address_type);
			knxclient_object_register(isolate, ldata_object, "hops", frame.payload.ldata.control2.hops);

			knxclient_object_register(isolate, ldata_object, "source", frame.payload.ldata.source);
			knxclient_object_register(isolate, ldata_object, "destination", frame.payload.ldata.destination);

			ldata_object->Set(String::NewFromUtf8(isolate, "tpdu"),
			                  knxclient_tpdu_to_object(isolate, frame.payload.ldata.tpdu));

			frame_object->Set(String::NewFromUtf8(isolate, "payload"), ldata_object);

			args.GetReturnValue().Set(frame_object);
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

	// Modules
	knxclient_register_knx_module(module);
	knxclient_register_knx_util(module);

	// Methods
	module->Set(String::NewFromUtf8(isolate, "parseCEMI"),
	            Function::New(isolate, knxclient_parse_cemi));
}

NODE_MODULE(knxclient, knxclient_init)
