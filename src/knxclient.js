var dgram = require("dgram");
var proto = require("bindings")("knxproto.node");

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

function RouterClient(conf) {
	conf = conf || {};

	// We need this configuration to send messages
	this.conf = {
		host: conf.host || "224.0.23.12",
		port: conf.port || 3671
	};

	// Create socket and add it to the router's multicast group
	this.sock = dgram.createSocket({type: "udp4", reuseAddr: true});
	this.sock.bind(this.conf.port, function () {
		this.sock.addMembership(this.conf.host);
		this.sock.setMulticastLoopback(false);
	}.bind(this));
	this.sock.unref();
}

RouterClient.prototype = {
	listen: function (callback) {
		this.sock.ref();
		this.sock.on("message", function (packet, sender) {
			var msg = proto.processRouted(packet);

			// The parser returns null, if the packet contents are invalid
			if (msg && msg.apci == 2) {
				msg.__proto__ = MessagePrototype;
				callback(sender, msg);
			}
		});
	},

	send: function (src, dest, payload) {
		var buf = proto.makeRouted(src, dest, payload);
		this.sock.send(buf, 0, buf.length, this.conf.port, this.conf.host);
	},

	close: function () {
		this.sock.dropMembership(this.conf.host);
		this.sock.close();
	}
};

module.exports = {
	// Clients
	RouterClient:            RouterClient,

	// Individual address
	formatIndividualAddress: formatIndividualAddress,
	makeIndividualAddress:   makeIndividualAddress,

	// Group address
	formatGroupAddress:      formatGroupAddress,
	makeGroupAddress:        makeGroupAddress,

	// Payload generators
	makeUnsigned8:           proto.makeUnsigned8,
	makeUnsigned16:          proto.makeUnsigned16,
	makeUnsigned32:          proto.makeUnsigned32,
	makeSigned8:             proto.makeSigned8,
	makeSigned16:            proto.makeSigned16,
	makeSigned32:            proto.makeSigned32,
	makeFloat16:             proto.makeFloat16,
	makeFloat32:             proto.makeFloat32,
	makeBool:                proto.makeBool,
	makeChar:                proto.makeChar,
	makeCValue:              proto.makeCValue,
	makeCStep:               proto.makeCStep,
	makeTimeOfDay:           proto.makeTimeOfDay,
	makeDate:                proto.makeDate
};
