#include "data.hpp"

#include <algorithm>
#include <node_buffer.h>

using namespace jawra;
using namespace v8;

static
void free_buffer(char* buffer, void*) {
	delete[] buffer;
}

static
Handle<Object> make_buffer(char* buffer, size_t length) {
	return node::Buffer::New(Isolate::GetCurrent(), buffer, length, free_buffer, nullptr);
}

#define KNXPROTO_PARSE_APDU_DEF(n, c, r) KNXPROTO_PARSE_APDU_DECL(n) { \
	knx_##n value; \
	\
	if (!payload.data || payload.length < knx_dpt_size(c) || \
	    !knx_dpt_from_apdu((const uint8_t*) payload.data, payload.length, c, &value)) \
			return Null(Isolate::GetCurrent()); \
	\
	return pack<r>(Isolate::GetCurrent(), value); \
}

KNXPROTO_PARSE_APDU_DEF(unsigned8,  KNX_DPT_UNSIGNED8,  uint32_t)
KNXPROTO_PARSE_APDU_DEF(unsigned16, KNX_DPT_UNSIGNED16, uint32_t)
KNXPROTO_PARSE_APDU_DEF(unsigned32, KNX_DPT_UNSIGNED32, uint32_t)

KNXPROTO_PARSE_APDU_DEF(signed8,    KNX_DPT_SIGNED8,    int32_t)
KNXPROTO_PARSE_APDU_DEF(signed16,   KNX_DPT_SIGNED16,   int32_t)
KNXPROTO_PARSE_APDU_DEF(signed32,   KNX_DPT_SIGNED32,   int32_t)

KNXPROTO_PARSE_APDU_DEF(float16,    KNX_DPT_FLOAT16,    double)
KNXPROTO_PARSE_APDU_DEF(float32,    KNX_DPT_FLOAT32,    double)

KNXPROTO_PARSE_APDU_DEF(bool,       KNX_DPT_BOOL,       bool)
KNXPROTO_PARSE_APDU_DEF(char,       KNX_DPT_CHAR,       char)

KNXPROTO_PARSE_APDU_DEF(cvalue,     KNX_DPT_CVALUE,     knx_cvalue)
KNXPROTO_PARSE_APDU_DEF(cstep,      KNX_DPT_CSTEP,      knx_cstep)
KNXPROTO_PARSE_APDU_DEF(timeofday,  KNX_DPT_TIMEOFDAY,  knx_timeofday)
KNXPROTO_PARSE_APDU_DEF(date,       KNX_DPT_DATE,       knx_date)

#define KNXPROTO_MAKE_APDU_DEF_S(n, c, t) KNXPROTO_MAKE_APDU_DECL(n, t) { \
	size_t length = knx_dpt_size(c); \
	char* buffer = new char[length]; \
	std::fill(buffer, buffer + length, 0); \
	knx_dpt_to_apdu((uint8_t*) buffer, c, &value); \
	return make_buffer(buffer, length); \
}

#define KNXPROTO_MAKE_APDU_DEF(n, c, t) KNXPROTO_MAKE_APDU_DECL(n, t) { \
	size_t length = knx_dpt_size(c); \
	char* buffer = new char[length]; \
	std::fill(buffer, buffer + length, 0); \
	knx_##n value2 = (knx_##n) value; \
	knx_dpt_to_apdu((uint8_t*) buffer, c, &value2); \
	return make_buffer(buffer, length); \
}

KNXPROTO_MAKE_APDU_DEF(unsigned8,  KNX_DPT_UNSIGNED8,  uint32_t)
KNXPROTO_MAKE_APDU_DEF(unsigned16, KNX_DPT_UNSIGNED16, uint32_t)
KNXPROTO_MAKE_APDU_DEF(unsigned32, KNX_DPT_UNSIGNED32, uint32_t)

KNXPROTO_MAKE_APDU_DEF(signed8,    KNX_DPT_SIGNED8,    int32_t)
KNXPROTO_MAKE_APDU_DEF(signed16,   KNX_DPT_SIGNED16,   int32_t)
KNXPROTO_MAKE_APDU_DEF(signed32,   KNX_DPT_SIGNED32,   int32_t)

KNXPROTO_MAKE_APDU_DEF(float16,   KNX_DPT_FLOAT16,    double)
KNXPROTO_MAKE_APDU_DEF(float32,   KNX_DPT_FLOAT32,    double)

KNXPROTO_MAKE_APDU_DEF(bool,       KNX_DPT_BOOL,       bool)
KNXPROTO_MAKE_APDU_DEF(char,       KNX_DPT_CHAR,       char)

KNXPROTO_MAKE_APDU_DEF_S(cvalue,     KNX_DPT_CVALUE,     knx_cvalue)
KNXPROTO_MAKE_APDU_DEF_S(cstep,      KNX_DPT_CSTEP,      knx_cstep)
KNXPROTO_MAKE_APDU_DEF_S(timeofday,  KNX_DPT_TIMEOFDAY,  knx_timeofday)
KNXPROTO_MAKE_APDU_DEF_S(date,       KNX_DPT_DATE,       knx_date)
