#ifndef KNXPROTO_NODE_DATA_H_
#define KNXPROTO_NODE_DATA_H_

extern "C" {
	#include <knxproto/proto/data.h>
}

#include <jawra/objects.hpp>
#include <jawra/functions.hpp>
#include <jawra/values.hpp>

void knxproto_free_buffer(char* buffer, void*);

v8::Handle<v8::Value> knxproto_make_buffer(char* buffer, size_t length);

namespace jawra {
	template <>
	struct ValueWrapper<char> {
		static
		constexpr const char* TypeName = "character";

		static inline
		bool check(v8::Handle<v8::Value> value) {
			return value->IsString() && value->ToString()->Length() > 0;
		}

		static inline
		char unpack(v8::Handle<v8::Value> value) {
			v8::String::Utf8Value strval(value);
			return (*strval)[0];
		}

		static inline
		v8::Local<v8::String> pack(v8::Isolate* isolate, char value) {
			return v8::String::NewFromOneByte(isolate, (const uint8_t*) &value, v8::String::kNormalString, 1);
		}
	};

	template <>
	struct ValueWrapper<knx_cvalue> {
		static
		constexpr const char* TypeName = "KNX cvalue";

		static inline
		bool check(v8::Handle<v8::Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return value_wrap.expect<bool>("control") && value_wrap.expect<bool>("value");
		}

		static inline
		knx_cvalue unpack(v8::Handle<v8::Value> value) {
			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return {
				value_wrap.get<bool>("control"),
				value_wrap.get<bool>("value")
			};
		}

		static inline
		v8::Local<v8::Object> pack(v8::Isolate* isolate, const knx_cvalue& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("control", value.control);
			value_wrap.set("value", value.value);

			return value_wrap;
		}
	};

	template <>
	struct ValueWrapper<knx_cstep> {
		static
		constexpr const char* TypeName = "KNX cstep";

		static inline
		bool check(v8::Handle<v8::Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return value_wrap.expect<bool>("control") && value_wrap.expect<uint32_t>("step");
		}

		static inline
		knx_cstep unpack(v8::Handle<v8::Value> value) {
			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return {
				value_wrap.get<bool>("control"),
				uint8_t(value_wrap.get<uint32_t>("step"))
			};
		}

		static inline
		v8::Local<v8::Object> pack(v8::Isolate* isolate, const knx_cstep& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("control", value.control);
			value_wrap.set("step", uint32_t(value.step));

			return value_wrap;
		}
	};

	template <>
	struct ValueWrapper<knx_timeofday> {
		static
		constexpr const char* TypeName = "KNX time-of-day";

		static inline
		bool check(v8::Handle<v8::Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return
				value_wrap.expect<uint32_t>("day") &&
				value_wrap.expect<uint32_t>("hour") &&
				value_wrap.expect<uint32_t>("minute") &&
				value_wrap.expect<uint32_t>("second");
		}

		static inline
		knx_timeofday unpack(v8::Handle<v8::Value> value) {
			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return {
				knx_dayofweek(value_wrap.get<bool>("day")),
				uint8_t(value_wrap.get<uint32_t>("hour")),
				uint8_t(value_wrap.get<uint32_t>("minute")),
				uint8_t(value_wrap.get<uint32_t>("second"))
			};
		}

		static inline
		v8::Local<v8::Object> pack(v8::Isolate* isolate, const knx_timeofday& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("day", uint32_t(value.day));
			value_wrap.set("hour", uint32_t(value.hour));
			value_wrap.set("minute", uint32_t(value.minute));
			value_wrap.set("second", uint32_t(value.second));

			return value_wrap;
		}
	};

	template <>
	struct ValueWrapper<knx_date> {
		static
		constexpr const char* TypeName = "KNX date";

		static inline
		bool check(v8::Handle<v8::Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return
				value_wrap.expect<uint32_t>("day") &&
				value_wrap.expect<uint32_t>("month") &&
				value_wrap.expect<uint32_t>("year");
		}

		static inline
		knx_date unpack(v8::Handle<v8::Value> value) {
			ObjectWrapper value_wrap(v8::Isolate::GetCurrent(), value->ToObject());

			return {
				uint8_t(value_wrap.get<uint32_t>("day")),
				uint8_t(value_wrap.get<uint32_t>("month")),
				uint8_t(value_wrap.get<uint32_t>("year"))
			};
		}

		static inline
		v8::Local<v8::Object> pack(v8::Isolate* isolate, const knx_date& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("day",   uint32_t(value.day));
			value_wrap.set("month", uint32_t(value.month));
			value_wrap.set("year",  uint32_t(value.year));

			return value_wrap;
		}
	};
}

#define KNXPROTO_PARSE_APDU_DECL(n) v8::Handle<v8::Value> knxproto_parse_##n(jawra::Buffer payload)

KNXPROTO_PARSE_APDU_DECL(unsigned8);
KNXPROTO_PARSE_APDU_DECL(unsigned16);
KNXPROTO_PARSE_APDU_DECL(unsigned32);

KNXPROTO_PARSE_APDU_DECL(signed8);
KNXPROTO_PARSE_APDU_DECL(signed16);
KNXPROTO_PARSE_APDU_DECL(signed32);

KNXPROTO_PARSE_APDU_DECL(float16);
KNXPROTO_PARSE_APDU_DECL(float32);

KNXPROTO_PARSE_APDU_DECL(bool);
KNXPROTO_PARSE_APDU_DECL(char);

KNXPROTO_PARSE_APDU_DECL(cvalue);
KNXPROTO_PARSE_APDU_DECL(cstep);
KNXPROTO_PARSE_APDU_DECL(timeofday);
KNXPROTO_PARSE_APDU_DECL(date);

#define KNXPROTO_MAKE_APDU_DECL(n, t) v8::Handle<v8::Value> knxproto_make_##n(t value)

KNXPROTO_MAKE_APDU_DECL(unsigned8, uint32_t);
KNXPROTO_MAKE_APDU_DECL(unsigned16, uint32_t);
KNXPROTO_MAKE_APDU_DECL(unsigned32, uint32_t);

KNXPROTO_MAKE_APDU_DECL(signed8, int32_t);
KNXPROTO_MAKE_APDU_DECL(signed16, int32_t);
KNXPROTO_MAKE_APDU_DECL(signed32, int32_t);

KNXPROTO_MAKE_APDU_DECL(float16, double);
KNXPROTO_MAKE_APDU_DECL(float32, double);

KNXPROTO_MAKE_APDU_DECL(bool, bool);
KNXPROTO_MAKE_APDU_DECL(char, char);

KNXPROTO_MAKE_APDU_DECL(cvalue, knx_cvalue);
KNXPROTO_MAKE_APDU_DECL(cstep, knx_cstep);
KNXPROTO_MAKE_APDU_DECL(timeofday, knx_timeofday);
KNXPROTO_MAKE_APDU_DECL(date, knx_date);

#endif
