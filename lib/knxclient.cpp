#include "knx.hpp"

extern "C" {
	#include <knxclient/proto/cemi.h>
}

#include <node.h>
#include <node_buffer.h>

using namespace v8;

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

			ldata_object->Set(String::NewFromUtf8(isolate, "priority"),
			                  Integer::New(isolate, frame.payload.ldata.control1.priority));
			ldata_object->Set(String::NewFromUtf8(isolate, "repeat"),
			                  Boolean::New(isolate, frame.payload.ldata.control1.repeat));
			ldata_object->Set(String::NewFromUtf8(isolate, "system_broadcast"),
			                  Boolean::New(isolate, frame.payload.ldata.control1.system_broadcast));
			ldata_object->Set(String::NewFromUtf8(isolate, "request_ack"),
			                  Boolean::New(isolate, frame.payload.ldata.control1.request_ack));
			ldata_object->Set(String::NewFromUtf8(isolate, "error"),
			                  Boolean::New(isolate, frame.payload.ldata.control1.error));

			ldata_object->Set(String::NewFromUtf8(isolate, "address_type"),
			                  Integer::New(isolate, frame.payload.ldata.control2.address_type));
			ldata_object->Set(String::NewFromUtf8(isolate, "hops"),
			                  Integer::New(isolate, frame.payload.ldata.control2.hops));

			ldata_object->Set(String::NewFromUtf8(isolate, "source"),
			                  Integer::New(isolate, frame.payload.ldata.source));
			ldata_object->Set(String::NewFromUtf8(isolate, "destination"),
			                  Integer::New(isolate, frame.payload.ldata.destination));

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

	// Constants
	knxclient_register_knx_module(module);

	// Methods
	module->Set(String::NewFromUtf8(isolate, "parseCEMI"),
	            Function::New(isolate, knxclient_parse_cemi));
}

NODE_MODULE(knxclient, knxclient_init)
