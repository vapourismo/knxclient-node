var EventEmitter = require("events");
var dgram        = require("dgram");
var proto        = require("bindings")("knxproto.node");

///////////////
// Utilities //
///////////////

function unpackGroup(addr) {
	return [(addr >> 11) & 15, (addr >> 8) & 7, addr & 255];
}

function packGroup(a, b, c) {
	return (
		((a & 15) << 11) |
		((b & 7) << 8) |
		(c & 255)
	);
}

function unpackIndividual(addr) {
	return [(addr >> 12) & 15, (addr >> 8) & 15, addr & 255];
}

function packIndividual(a, b, c) {
	return (
		((a & 15) << 12) |
		((b & 15) << 8) |
		(c & 255)
	);
}

///////////////////
// Router client //
///////////////////

function Router(host, port) {
	EventEmitter.prototype.constructor.call(this);

	this.host = host || "224.0.23.12",
	this.port = port || 3671

	this.ext = proto.createRouter(
		function (buf) {
			this.sock.send(buf, 0, buf.length, this.port, this.host);
		}.bind(this),

		function (msg) {
			if (!msg) return;

			this.emit("message", msg);

			if (msg.service == proto.LDataIndication)
				this.emit("indication", msg.payload.source, msg.payload.destination, msg.payload.tpdu);
			else if (msg.service == proto.LDataConfirmation)
				this.emit("confirmation", msg.payload.source, msg.payload.destination, msg.payload.tpdu);
		}.bind(this)
	);

	this.sock = dgram.createSocket({type: "udp4", reuseAddr: true});
	this.sock.bind(this.port, function () {
		this.sock.addMembership(this.host);
		this.sock.setMulticastLoopback(false);
	}.bind(this));

	this.sock.on("message", function (msg) {
		if (this.ext) proto.processRouter(this.ext, msg);
	}.bind(this));

	this.sock.on("close", function () {
		if (!this.ext) return;

		proto.disposeRouter(this.ext);
		this.ext = null;
	}.bind(this));
}

Router.prototype.__proto__ = EventEmitter.prototype;

Router.prototype.send = function (cemi) {
	if (this.ext) return proto.sendRouter(this.ext, cemi);
};

Router.prototype.write = function (src, dest, payload) {
	return this.send({
		service: proto.LDataIndication,
		payload: {
			source: src,
			destination: dest,
			tpdu: {
				tpci: proto.UnnumberedData,
				apci: proto.GroupValueWrite,
				payload: payload
			}
		}
	});
};

Router.prototype.dispose = function () {
	this.sock.dropMembership(this.host);
	this.sock.close();
};

///////////////////
// Tunnel client //
///////////////////

function Tunnel(host, port) {
	EventEmitter.prototype.constructor.call(this);

	this.host = host || "localhost";
	this.port = port || 3671;

	this.ext = proto.createTunnel(
		this.emit.bind(this, "state"),

		function (buf) {
			this.sock.send(buf, 0, buf.length, this.port, this.host);
		}.bind(this),

		function (msg) {
			if (!msg) return;

			this.emit("message", msg);

			if (msg.service == proto.LDataIndication)
				this.emit("indication", msg.payload.source, msg.payload.destination, msg.payload.tpdu);
			else if (msg.service == proto.LDataConfirmation)
				this.emit("confirmation", msg.payload.source, msg.payload.destination, msg.payload.tpdu);
		}.bind(this),

		this.emit.bind(this, "ack")
	);

	this.sock = dgram.createSocket({type: "udp4", reuseAddr: true});

	this.sock.on("message", function (msg) {
		if (this.ext) proto.processTunnel(this.ext, msg);
	}.bind(this));

	this.sock.on("close", function () {
		if (!this.ext) return;

		proto.disposeTunnel(this.ext);
		this.ext = null;
	}.bind(this));
}

Tunnel.prototype.__proto__ = EventEmitter.prototype;

Tunnel.prototype.connect = function () {
	if (this.ext) proto.connectTunnel(this.ext);
};

Tunnel.prototype.disconnect = function () {
	if (this.ext) proto.disconnectTunnel(this.ext);
};

Tunnel.prototype.dispose = function () {
	this.sock.close();
};

Tunnel.prototype.send = function (cemi) {
	if (this.ext) return proto.sendTunnel(this.ext, cemi);
}

Tunnel.prototype.write = function (src, dest, payload) {
	return this.send({
		service: proto.LDataRequest,
		payload: {
			source: src,
			destination: dest,
			tpdu: {
				tpci: proto.UnnumberedData,
				apci: proto.GroupValueWrite,
				payload: payload
			}
		}
	});
};

/////////////
// Exports //
/////////////

module.exports = {
	// Clients
	Router:                 Router,
	Tunnel:                 Tunnel,

	// Address
	packIndividual:         packIndividual,
	packGroup:              packGroup,

	unpackIndividual:       unpackIndividual,
	unpackGroup:            unpackGroup,

	// Data types
	unpackUnsigned8:        proto.unpackUnsigned8,
	unpackUnsigned16:       proto.unpackUnsigned16,
	unpackUnsigned32:       proto.unpackUnsigned32,
	unpackSigned8:          proto.unpackSigned8,
	unpackSigned16:         proto.unpackSigned16,
	unpackSigned32:         proto.unpackSigned32,
	unpackFloat16:          proto.unpackFloat16,
	unpackFloat32:          proto.unpackFloat32,
	unpackBool:             proto.unpackBool,
	unpackChar:             proto.unpackChar,
	unpackCValue:           proto.unpackCValue,
	unpackCStep:            proto.unpackCStep,
	unpackTimeOfDay:        proto.unpackTimeOfDay,
	unpackDate:             proto.unpackDate,

	packUnsigned8:          proto.packUnsigned8,
	packUnsigned16:         proto.packUnsigned16,
	packUnsigned32:         proto.packUnsigned32,
	packSigned8:            proto.packSigned8,
	packSigned16:           proto.packSigned16,
	packSigned32:           proto.packSigned32,
	packFloat16:            proto.packFloat16,
	packFloat32:            proto.packFloat32,
	packBool:               proto.packBool,
	packChar:               proto.packChar,
	packCValue:             proto.packCValue,
	packCStep:              proto.packCStep,
	packTimeOfDay:          proto.packTimeOfDay,
	packDate:               proto.packDate,

	// CEMI Constants
	LDataRequest:           proto.LDataRequest,
	LDataConfirmation:      proto.LDataConfirmation,
	LDataIndication:        proto.LDataIndication,

	// L_Data constants
	NumberedData:           proto.NumberedData,
	UnnumberedData:         proto.UnnumberedData,
	NumberedControl:        proto.NumberedControl,
	UnnumberedControl:      proto.UnnumberedControl,

	GroupValueRead:         proto.GroupValueRead,
	GroupValueResponse:     proto.GroupValueResponse,
	GroupValueWrite:        proto.GroupValueWrite,
	IndividualAddrWrite:    proto.IndividualAddrWrite,
	IndividualAddrRequest:  proto.IndividualAddrRequest,
	IndividualAddrResponse: proto.IndividualAddrResponse,
	ADCRead:                proto.ADCRead,
	ADCResponse:            proto.ADCResponse,
	MemoryRead:             proto.MemoryRead,
	MemoryResponse:         proto.MemoryResponse,
	MemoryWrite:            proto.MemoryWrite,
	UserMessage:            proto.UserMessage,
	MaskVersionRead:        proto.MaskVersionRead,
	MaskVersionResponse:    proto.MaskVersionResponse,
	Restart:                proto.Restart,
	Escape:                 proto.Escape
};
