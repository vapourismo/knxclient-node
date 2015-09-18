#include "data.hpp"

extern "C" {
	#include <knxproto/proto/data.h>
}

using namespace jawra;
using namespace v8;

namespace jawra {
	template <>
	struct ValueWrapper<char> {
		static
		constexpr const char* TypeName = "character";

		static inline
		bool check(Handle<Value> value) {
			return value->IsString() && value->ToString()->Length() > 0;
		}

		static inline
		char unpack(Handle<Value> value) {
			String::Utf8Value strval(value);
			return (*strval)[0];
		}

		static inline
		Local<String> pack(Isolate* isolate, char value) {
			return String::NewFromOneByte(isolate, (const uint8_t*) &value, String::kNormalString, 1);
		}
	};

	template <>
	struct ValueWrapper<knx_cvalue> {
		static
		constexpr const char* TypeName = "KNX CValue";

		static inline
		bool check(Handle<Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return value_wrap.expect<bool>("control") && value_wrap.expect<bool>("value");
		}

		static inline
		knx_cvalue unpack(Handle<Value> value) {
			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return {
				value_wrap.get<bool>("control"),
				value_wrap.get<bool>("value")
			};
		}

		static inline
		Local<Object> pack(Isolate* isolate, const knx_cvalue& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("control", value.control);
			value_wrap.set("value", value.value);

			return value_wrap;
		}
	};

	template <>
	struct ValueWrapper<knx_cstep> {
		static
		constexpr const char* TypeName = "KNX CStep";

		static inline
		bool check(Handle<Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return value_wrap.expect<bool>("control") && value_wrap.expect<uint32_t>("step");
		}

		static inline
		knx_cstep unpack(Handle<Value> value) {
			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return {
				value_wrap.get<bool>("control"),
				uint8_t(value_wrap.get<uint32_t>("step"))
			};
		}

		static inline
		Local<Object> pack(Isolate* isolate, const knx_cstep& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("control", value.control);
			value_wrap.set("step", uint32_t(value.step));

			return value_wrap;
		}
	};

	template <>
	struct ValueWrapper<knx_timeofday> {
		static
		constexpr const char* TypeName = "KNX Time-of-day";

		static inline
		bool check(Handle<Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return
				value_wrap.expect<uint32_t>("day") &&
				value_wrap.expect<uint32_t>("hour") &&
				value_wrap.expect<uint32_t>("minute") &&
				value_wrap.expect<uint32_t>("second");
		}

		static inline
		knx_timeofday unpack(Handle<Value> value) {
			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return {
				knx_dayofweek(value_wrap.get<bool>("day")),
				uint8_t(value_wrap.get<uint32_t>("hour")),
				uint8_t(value_wrap.get<uint32_t>("minute")),
				uint8_t(value_wrap.get<uint32_t>("second"))
			};
		}

		static inline
		Local<Object> pack(Isolate* isolate, const knx_timeofday& value) {
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
		constexpr const char* TypeName = "KNX Time-of-day";

		static inline
		bool check(Handle<Value> value) {
			if (!value->IsObject())
				return false;

			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return
				value_wrap.expect<uint32_t>("day") &&
				value_wrap.expect<uint32_t>("month") &&
				value_wrap.expect<uint32_t>("year");
		}

		static inline
		knx_date unpack(Handle<Value> value) {
			ObjectWrapper value_wrap(Isolate::GetCurrent(), value->ToObject());

			return {
				uint8_t(value_wrap.get<uint32_t>("day")),
				uint8_t(value_wrap.get<uint32_t>("month")),
				uint8_t(value_wrap.get<uint32_t>("year"))
			};
		}

		static inline
		Local<Object> pack(Isolate* isolate, const knx_date& value) {
			ObjectWrapper value_wrap(isolate);

			value_wrap.set("day",   uint32_t(value.day));
			value_wrap.set("month", uint32_t(value.month));
			value_wrap.set("year",  uint32_t(value.year));

			return value_wrap;
		}
	};
}

#define KNXPROTO_APDU_DEF(n, c, r) KNXPROTO_APDU_DECL(n) { \
	knx_##n value; \
	\
	if (!payload.data || payload.length < knx_dpt_size(c) || \
	    !knx_dpt_from_apdu((const uint8_t*) payload.data, payload.length, c, &value)) \
			return Null(Isolate::GetCurrent()); \
	\
	return pack<r>(Isolate::GetCurrent(), value); \
}

KNXPROTO_APDU_DEF(unsigned8,  KNX_DPT_UNSIGNED8,  uint32_t)
KNXPROTO_APDU_DEF(unsigned16, KNX_DPT_UNSIGNED16, uint32_t)
KNXPROTO_APDU_DEF(unsigned32, KNX_DPT_UNSIGNED32, uint32_t)

KNXPROTO_APDU_DEF(signed8,    KNX_DPT_SIGNED8,    int32_t)
KNXPROTO_APDU_DEF(signed16,   KNX_DPT_SIGNED16,   int32_t)
KNXPROTO_APDU_DEF(signed32,   KNX_DPT_SIGNED32,   int32_t)

KNXPROTO_APDU_DEF(float16,    KNX_DPT_FLOAT16,    double)
KNXPROTO_APDU_DEF(float32,    KNX_DPT_FLOAT32,    double)

KNXPROTO_APDU_DEF(bool,       KNX_DPT_BOOL,       bool)
KNXPROTO_APDU_DEF(char,       KNX_DPT_CHAR,       char)

KNXPROTO_APDU_DEF(cvalue,     KNX_DPT_CVALUE,     knx_cvalue)
KNXPROTO_APDU_DEF(cstep,      KNX_DPT_CSTEP,      knx_cstep)
KNXPROTO_APDU_DEF(timeofday,  KNX_DPT_TIMEOFDAY,  knx_timeofday)
KNXPROTO_APDU_DEF(date,       KNX_DPT_DATE,       knx_date)
