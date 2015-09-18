#ifndef KNXPROTO_NODE_DATA_H_
#define KNXPROTO_NODE_DATA_H_

#include <jawra/objects.hpp>
#include <jawra/functions.hpp>
#include <jawra/values.hpp>

#define KNXPROTO_APDU_DECL(n) v8::Handle<v8::Value> knxproto_parse_##n(jawra::Buffer payload)

KNXPROTO_APDU_DECL(unsigned8);
KNXPROTO_APDU_DECL(unsigned16);
KNXPROTO_APDU_DECL(unsigned32);

KNXPROTO_APDU_DECL(signed8);
KNXPROTO_APDU_DECL(signed16);
KNXPROTO_APDU_DECL(signed32);

KNXPROTO_APDU_DECL(float16);
KNXPROTO_APDU_DECL(float32);

KNXPROTO_APDU_DECL(bool);
KNXPROTO_APDU_DECL(char);

KNXPROTO_APDU_DECL(cvalue);
KNXPROTO_APDU_DECL(cstep);
KNXPROTO_APDU_DECL(timeofday);
KNXPROTO_APDU_DECL(date);

#endif
