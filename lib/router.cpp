#include "router.hpp"

#include <memory>
#include <list>

extern "C" {
	#include <knxclient/routerclient.h>
}

using namespace v8;

struct RouterClientHandle {
	knx_router_client client;
	bool active = false;

	bool connect(const char* host, in_port_t port) {
		ip4addr mc_address;
		ip4addr_resolve(&mc_address, host, port);

		active = knx_router_connect(&client, &mc_address);
		return active;
	}

	~RouterClientHandle() {
		if (active) {
			knx_router_disconnect(&client);
		}
	}
};

static
std::list<std::unique_ptr<RouterClientHandle>> active_router_clients;

static
void router_ctor(const FunctionCallbackInfo<Value>& args) {
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

	// Allocate client
	RouterClientHandle* client = new RouterClientHandle;

	if (!client) {
		isolate->ThrowException(
			Exception::Error(
				String::NewFromUtf8(isolate, "Failed to allocate KNX router client")
			)
		);

		return;
	}

	// Connect client
	if (!client->connect(*host, port)) {
		delete client;

		isolate->ThrowException(
			Exception::Error(
				String::NewFromUtf8(isolate, "Failed to connect KNX router client")
			)
		);

		return;
	}

	// Construct object
	active_router_clients.emplace_back(client);
	Handle<Object> self = args.This();
	self->SetInternalField(0, External::New(isolate, client));
}

static
RouterClientHandle* router_check(const FunctionCallbackInfo<Value>& args) {
	Handle<Object> self = args.This();

	if (self->InternalFieldCount() != 1)
		return nullptr;

	Local<Value> field = self->GetInternalField(0);

	if (!field->IsExternal())
		return nullptr;

	return static_cast<RouterClientHandle*>(Handle<External>::Cast(field)->Value());
}

static
void router_send(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();

	// Fetch internal field
	RouterClientHandle* handle = router_check(args);

	if (!handle) {
		isolate->ThrowException(
			Exception::TypeError(
				String::NewFromUtf8(isolate, "Object 'this' is not a router client")
			)
		);

		return;
	}

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

	// Parse datapoint type
	uint32_t addr = args[0]->Uint32Value();
	uint32_t dpt = args[1]->Uint32Value();

	switch (dpt) {
		case KNX_DPT_BOOL:
			if (args.Length() >= 3 && args[2]->IsBoolean()) {
				knx_bool value = args[2]->BooleanValue();
				knx_router_write_group(&handle->client, addr, KNX_DPT_BOOL, &value);
				return;
			} else break;

		case KNX_DPT_CVALUE:
			if (args.Length() >= 4 && args[2]->IsBoolean() && args[3]->IsBoolean()) {
				knx_cvalue value = {
					.control = args[2]->BooleanValue(),
					.value = args[3]->BooleanValue()
				};
				knx_router_write_group(&handle->client, addr, KNX_DPT_CVALUE, &value);
				return;
			} else break;

		case KNX_DPT_CSTEP:
			if (args.Length() >= 4 && args[2]->IsBoolean() && args[3]->IsUint32()) {
				knx_cstep value = {
					.control = args[2]->BooleanValue(),
					.step = uint8_t(args[3]->Uint32Value())
				};
				knx_router_write_group(&handle->client, addr, KNX_DPT_CSTEP, &value);
				return;
			} else break;

		case KNX_DPT_UNSIGNED8:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned8 value = args[2]->Uint32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_UNSIGNED8, &value);
				return;
			} else break;

		case KNX_DPT_SIGNED8:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed8 value = args[2]->Int32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_SIGNED8, &value);
				return;
			} else break;

		case KNX_DPT_UNSIGNED16:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned16 value = args[2]->Uint32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_UNSIGNED16, &value);
			} else break;

		case KNX_DPT_SIGNED16:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed16 value = args[2]->Int32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_SIGNED16, &value);
				return;
			} else break;

		case KNX_DPT_UNSIGNED32:
			if (args.Length() >= 3 && args[2]->IsUint32()) {
				knx_unsigned32 value = args[2]->Uint32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_UNSIGNED32, &value);
				return;
			} else break;

		case KNX_DPT_SIGNED32:
			if (args.Length() >= 3 && args[2]->IsInt32()) {
				knx_signed32 value = args[2]->Int32Value();
				knx_router_write_group(&handle->client, addr, KNX_DPT_SIGNED32, &value);
				return;
			} else break;

		case KNX_DPT_FLOAT16:
			if (args.Length() >= 3 && args[2]->IsNumber()) {
				knx_float16 value = args[2]->NumberValue();
				knx_router_write_group(&handle->client, addr, KNX_DPT_FLOAT16, &value);
				return;
			} else break;

		case KNX_DPT_FLOAT32:
			if (args.Length() >= 3 && args[2]->IsNumber()) {
				knx_float32 value = args[2]->NumberValue();
				knx_router_write_group(&handle->client, addr, KNX_DPT_FLOAT32, &value);
				return;
			} else break;

		case KNX_DPT_TIMEOFDAY:
			if (args.Length() >= 6 && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32() && args[5]->IsUint32()) {
				knx_timeofday value = {
					.day = knx_dayofweek(args[2]->Uint32Value()),
					.hour = uint8_t(args[3]->Uint32Value()),
					.minute = uint8_t(args[4]->Uint32Value()),
					.second = uint8_t(args[5]->Uint32Value())
				};
				knx_router_write_group(&handle->client, addr, KNX_DPT_TIMEOFDAY, &value);
				return;
			} else break;

		case KNX_DPT_DATE:
			if (args.Length() >= 5 && args[2]->IsUint32() && args[3]->IsUint32() && args[4]->IsUint32()) {
				knx_date value = {
					.day = uint8_t(args[2]->Uint32Value()),
					.month = uint8_t(args[3]->Uint32Value()),
					.year = uint8_t(args[4]->Uint32Value())
				};
				knx_router_write_group(&handle->client, addr, KNX_DPT_DATE, &value);
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

static
void router_listen(const FunctionCallbackInfo<Value>& args) {

}

void knxclient_init_router(v8::Handle<v8::Object> module) {
	Isolate* isolate = Isolate::GetCurrent();

	// Constructor
	Local<FunctionTemplate> ctor = FunctionTemplate::New(isolate, router_ctor);
	ctor->SetClassName(String::NewFromUtf8(isolate, "Router"));
	ctor->InstanceTemplate()->SetInternalFieldCount(1);

	module->Set(String::NewFromUtf8(isolate, "Router"), ctor->GetFunction());

	// Methods
	NODE_SET_PROTOTYPE_METHOD(ctor, "send", router_send);

	// Special values
	module->Set(String::NewFromUtf8(isolate, "BooleanType"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_BOOL));
	module->Set(String::NewFromUtf8(isolate, "ControlValueType"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_CVALUE));
	module->Set(String::NewFromUtf8(isolate, "ControlStepType"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_CSTEP));
	module->Set(String::NewFromUtf8(isolate, "Unsigned8Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_UNSIGNED8));
	module->Set(String::NewFromUtf8(isolate, "Signed8Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_SIGNED8));
	module->Set(String::NewFromUtf8(isolate, "Unsigned16Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_UNSIGNED16));
	module->Set(String::NewFromUtf8(isolate, "Signed16Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_SIGNED16));
	module->Set(String::NewFromUtf8(isolate, "Unsigned32Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_UNSIGNED32));
	module->Set(String::NewFromUtf8(isolate, "Signed32Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_SIGNED32));
	module->Set(String::NewFromUtf8(isolate, "Float16Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_FLOAT16));
	module->Set(String::NewFromUtf8(isolate, "Float32Type"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_FLOAT32));
	module->Set(String::NewFromUtf8(isolate, "TimeOfDayType"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_TIMEOFDAY));
	module->Set(String::NewFromUtf8(isolate, "DateType"),
	            Integer::NewFromUnsigned(isolate, KNX_DPT_DATE));
}
