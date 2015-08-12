#include <iostream>
#include <node.h>
#include <node_object_wrap.h>

extern "C" {
#include <knxclient/routerclient.h>
}

using namespace v8;

void knxclient_router_ctor(const FunctionCallbackInfo<Value>& args) {
	if (!args.IsConstructCall())
		return;

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	// Type check
	if (args.Length() < 2) {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Wrong number of arguments")
			)
		);

		return;
	} else if (!args[0]->IsString() || !args[1]->IsUint32()) {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Wrong type of arguments")
			)
		);

		return;
	}

	// Extract arguments
	String::Utf8Value host(args[0]);
	in_port_t port = args[1]->Uint32Value();

	ip4addr mcaddr;
	ip4addr_resolve(&mcaddr, *host, port);

	// Allocate client
	knx_router_client* tunnel_client = new knx_router_client;

	if (tunnel_client == nullptr) {
		isolate->ThrowException(
			Exception::Error(
				String::NewFromUtf8(isolate, "Failed to allocate KNX router client")
			)
		);

		return;
	}

	// Connect client
	if (!knx_router_connect(tunnel_client, &mcaddr)) {
		isolate->ThrowException(
			Exception::Error(
				String::NewFromUtf8(isolate, "Failed to connect KNX router client")
			)
		);

		return;
	}

	// Construct object
	Handle<Object> self = args.This();
	self->SetInternalField(0, External::New(isolate, tunnel_client));
}

void knxclient_router_send(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	// Type check
	if (args.Length() < 2) {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Wrong number of arguments")
			)
		);

		return;
	} else if (!args[0]->IsUint32() || !args[1]->IsUint32()) {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Wrong type of arguments")
			)
		);

		return;
	}

	Local<External> inner = Local<External>::Cast(args.This()->GetInternalField(0));
	knx_router_client* client = static_cast<knx_router_client*>(inner->Value());

	uint32_t addr = args[0]->Uint32Value();
	uint32_t dpt = args[1]->Uint32Value();

	switch (dpt) {
		case 1:
			if (args.Length() >= 3 && args[2]->IsBoolean()) {
				knx_bool value = args[2]->BooleanValue();
				knx_router_write_group(client, addr, KNX_DPT_BOOL, &value);
				return;
			} else break;

		case 2:
			if (args.Length() >= 4 && args[2]->IsBoolean() && args[3]->IsBoolean()) {
				knx_cvalue value = {
					.control = args[2]->BooleanValue(),
					.value = args[3]->BooleanValue()
				};
				knx_router_write_group(client, addr, KNX_DPT_CVALUE, &value);
				return;
			} else break;

		case 3:
			if (args.Length() >= 4 && args[2]->IsBoolean() && args[3]->IsUint32()) {
				knx_cstep value = {
					.control = args[2]->BooleanValue(),
					.step = uint8_t(args[3]->Uint32Value())
				};
				knx_router_write_group(client, addr, KNX_DPT_CSTEP, &value);
				return;
			} else break;

		case 5:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned8 value = args[2]->Uint32Value();
				knx_router_write_group(client, addr, KNX_DPT_UNSIGNED8, &value);
				return;
			} else break;

		case 6:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed8 value = args[2]->Int32Value();
				knx_router_write_group(client, addr, KNX_DPT_SIGNED8, &value);
				return;
			} else break;

		case 7:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned16 value = args[2]->Uint32Value();
				knx_router_write_group(client, addr, KNX_DPT_UNSIGNED16, &value);
			} else break;

		case 8:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed16 value = args[2]->Int32Value();
				knx_router_write_group(client, addr, KNX_DPT_SIGNED16, &value);
				return;
			} else break;

		case 12:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned32 value = args[2]->Uint32Value();
				knx_router_write_group(client, addr, KNX_DPT_UNSIGNED32, &value);
				return;
			} else break;

		case 13:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed32 value = args[2]->Int32Value();
				knx_router_write_group(client, addr, KNX_DPT_SIGNED32, &value);
				return;
			} else break;

		case 9:
			if (args.Length() >= 3 && args[2]->IsNumber()) {
				knx_float16 value = args[2]->NumberValue();
				knx_router_write_group(client, addr, KNX_DPT_FLOAT16, &value);
				return;
			} else break;

		case 14:
			if (args.Length() >= 3 && args[2]->IsNumber()) {
				knx_float32 value = args[2]->NumberValue();
				knx_router_write_group(client, addr, KNX_DPT_FLOAT32, &value);
				return;
			} else break;

		case 10:
			if (args.Length() >= 6 && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32()) {
				knx_timeofday value = {
					.day = knx_dayofweek(args[2]->Uint32Value()),
					.hour = uint8_t(args[3]->Uint32Value()),
					.minute = uint8_t(args[4]->Uint32Value()),
					.second = uint8_t(args[5]->Uint32Value())
				};
				knx_router_write_group(client, addr, KNX_DPT_TIMEOFDAY, &value);
				return;
			} else break;

		case 11:
			if (args.Length() >= 5 && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32()) {
				knx_date value = {
					.day = uint8_t(args[2]->Uint32Value()),
					.month = uint8_t(args[3]->Uint32Value()),
					.year = uint8_t(args[4]->Uint32Value())
				};
				knx_router_write_group(client, addr, KNX_DPT_DATE, &value);
				return;
			} else break;

		default:
			isolate->ThrowException(
				Exception::TypeError(
					String::NewFromUtf8(isolate, "Unknown datapoint type identifier")
				)
			);

			return;
	}

	isolate->ThrowException(
		Exception::TypeError(
			String::NewFromUtf8(isolate, "Wrong type or number of datapoint arguments")
		)
	);
}

void knxclient_router_receive(const FunctionCallbackInfo<Value>& args) {

}

void knxclient_init(Handle<Object> exports) {
	Isolate* isolate = Isolate::GetCurrent();

	// Router
	Local<FunctionTemplate> routerCtor = FunctionTemplate::New(isolate, knxclient_router_ctor);
	routerCtor->SetClassName(String::NewFromUtf8(isolate, "Router"));
	routerCtor->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(routerCtor, "send", knxclient_router_send);
	NODE_SET_PROTOTYPE_METHOD(routerCtor, "receive", knxclient_router_receive);

	// Set exports
	exports->Set(String::NewFromUtf8(isolate, "Router"), routerCtor->GetFunction());
}

NODE_MODULE(knxclient, knxclient_init)
