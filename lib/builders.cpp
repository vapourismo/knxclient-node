#include "builders.hpp"

using namespace v8;

ObjectWrapper knxclient_build_host_info(Isolate* isolate, const knx_host_info& info) {
	ObjectWrapper builder(isolate);

	builder.set("protocol", info.protocol);
	builder.set("port", info.port);

	char address_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &info.address, address_str, INET_ADDRSTRLEN)) {
		builder.set("address", address_str);
	}

	return builder;
}

ObjectWrapper knxclient_build_tpdu(Isolate* isolate, const knx_tpdu& tpdu) {
	ObjectWrapper builder(isolate);

	builder.set("tpci", tpdu.tpci);
	builder.set("seq_number", tpdu.seq_number);

	switch (tpdu.tpci) {
		case KNX_TPCI_UNNUMBERED_DATA:
		case KNX_TPCI_NUMBERED_DATA:
			builder.set("apci", tpdu.info.data.apci);
			builder.set("payload", ObjectWrapper::fromData(isolate, (const char*) tpdu.info.data.payload, tpdu.info.data.length));
			break;

		case KNX_TPCI_UNNUMBERED_CONTROL:
		case KNX_TPCI_NUMBERED_CONTROL:
			builder.set("control", tpdu.info.control);
			break;
	}

	return builder;
}

ObjectWrapper knxclient_build_ldata(Isolate* isolate, const knx_ldata& ldata) {
	ObjectWrapper builder(isolate);

	builder.set("priority",        ldata.control1.priority);
	builder.set("repeat",          ldata.control1.repeat);
	builder.set("systemBroadcast", ldata.control1.system_broadcast);
	builder.set("requestAck",      ldata.control1.request_ack);
	builder.set("error",           ldata.control1.error);
	builder.set("addressType",     ldata.control2.address_type);
	builder.set("hops",            ldata.control2.hops);
	builder.set("source",          ldata.source);
	builder.set("destination",     ldata.destination);
	builder.set("tpdu",            knxclient_build_tpdu(isolate, ldata.tpdu));

	return builder;
}

ObjectWrapper knxclient_build_cemi(Isolate* isolate, const knx_cemi& cemi) {
	ObjectWrapper builder(isolate);

	builder.set("service", cemi.service);

	switch (cemi.service) {
		case KNX_CEMI_LDATA_REQ:
		case KNX_CEMI_LDATA_IND:
		case KNX_CEMI_LDATA_CON:
			builder.set("payload", knxclient_build_ldata(isolate, cemi.payload.ldata));
			break;
	}

	return builder;
}

ObjectWrapper knxclient_build_packet(Isolate* isolate, const knx_packet& ind) {
	ObjectWrapper builder(isolate);
	builder.set("service", ind.service);

	switch (ind.service) {
		case KNX_CONNECTION_RESPONSE: {
			auto& res = ind.payload.conn_res;

			builder.set("channel", res.channel);
			builder.set("status", res.status);
			builder.set("host", knxclient_build_host_info(isolate, res.host));

			break;
		}

		case KNX_DISCONNECT_REQUEST: {
			auto& req = ind.payload.dc_req;

			builder.set("channel", req.channel);
			builder.set("status", req.status);
			builder.set("host", knxclient_build_host_info(isolate, req.host));

			break;
		}

		case KNX_TUNNEL_REQUEST: {
			auto& req = ind.payload.tunnel_req;

			builder.set("channel", req.channel);
			builder.set("seqNumber", req.seq_number);
			builder.set("data", knxclient_build_cemi(isolate, req.data));

			break;
		}

		case KNX_ROUTING_INDICATION:
			builder.set("data", knxclient_build_cemi(isolate, ind.payload.routing_ind.data));
			break;

		default:
			break;
	}

	return builder;
}
