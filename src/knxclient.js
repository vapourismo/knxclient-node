var EventEmitter = require("events");
var dgram        = require("dgram");
var proto        = require("bindings")("knxproto.node");

///////////////
// Utilities //
///////////////

function formatGroupAddress(addr) {
	var a = (addr >> 11) & 15;
	var b = (addr >> 8) & 7
	var c = addr & 255;

	return a + "/" + b + "/" + c;
}

function makeGroupAddress(a, b, c) {
	return (
		((a & 15) << 11) |
		((b & 7) << 8) |
		(c & 255)
	);
}

function formatIndividualAddress(addr) {
	var a = (addr >> 12) & 15;
	var b = (addr >> 8) & 15
	var c = addr & 255;

	return a + "." + b + "." + c;
}

function makeIndividualAddress(a, b, c) {
	return (
		((a & 15) << 12) |
		((b & 15) << 8) |
		(c & 255)
	);
}

var MessagePrototype = {
	asUnsigned8:  function () { return proto.parseUnsigned8(this.data); },
	asUnsigned16: function () { return proto.parseUnsigned16(this.data); },
	asUnsigned32: function () { return proto.parseUnsigned32(this.data); },
	asSigned8:    function () { return proto.parseSigned8(this.data); },
	asSigned16:   function () { return proto.parseSigned16(this.data); },
	asSigned32:   function () { return proto.parseSigned32(this.data); },
	asFloat16:    function () { return proto.parseFloat16(this.data); },
	asFloat32:    function () { return proto.parseFloat32(this.data); },
	asBool:       function () { return proto.parseBool(this.data); },
	asChar:       function () { return proto.parseChar(this.data); },
	asCValue:     function () { return proto.parseCValue(this.data); },
	asCStep:      function () { return proto.parseCStep(this.data); },
	asTimeOfDay:  function () { return proto.parseTimeOfDay(this.data); },
	asDate:       function () { return proto.parseDate(this.data); }
};

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

			msg.__proto__ = MessagePrototype;
			this.emit("message", msg);
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

Router.prototype.write = function (src, dest, payload) {
	if (this.ext) return proto.writeRouter(this.ext, src, dest, payload);
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

			msg.__proto__ = MessagePrototype;
			this.emit("message", msg);
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

Tunnel.prototype.write = function (src, dest, payload, ack) {
	if (this.ext) return proto.writeTunnel(this.ext, src, dest, payload, !!ack);
};

/////////////
// Exports //
/////////////

module.exports = {
	Router:                  Router,
	Tunnel:                  Tunnel,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress:      formatGroupAddress,
	IndividualAddress:       makeIndividualAddress,
	GroupAddress:            makeGroupAddress,
	Unsigned8:               proto.makeUnsigned8,
	Unsigned16:              proto.makeUnsigned16,
	Unsigned32:              proto.makeUnsigned32,
	Signed8:                 proto.makeSigned8,
	Signed16:                proto.makeSigned16,
	Signed32:                proto.makeSigned32,
	Float16:                 proto.makeFloat16,
	Float32:                 proto.makeFloat32,
	Bool:                    proto.makeBool,
	Char:                    proto.makeChar,
	CValue:                  proto.makeCValue,
	CStep:                   proto.makeCStep,
	TimeOfDay:               proto.makeTimeOfDay,
	Date:                    proto.makeDate,
	LDataRequest:            proto.LDataRequest,
	LDataConfirmation:       proto.LDataConfirmation,
	LDataIndication:         proto.LDataIndication,
};
