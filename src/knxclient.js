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
	asUnsigned8:  function () { return proto.parseUnsigned8(this.payload); },
	asUnsigned16: function () { return proto.parseUnsigned16(this.payload); },
	asUnsigned32: function () { return proto.parseUnsigned32(this.payload); },
	asSigned8:    function () { return proto.parseSigned8(this.payload); },
	asSigned16:   function () { return proto.parseSigned16(this.payload); },
	asSigned32:   function () { return proto.parseSigned32(this.payload); },
	asFloat16:    function () { return proto.parseFloat16(this.payload); },
	asFloat32:    function () { return proto.parseFloat32(this.payload); },
	asBool:       function () { return proto.parseBool(this.payload); },
	asChar:       function () { return proto.parseChar(this.payload); },
	asCValue:     function () { return proto.parseCValue(this.payload); },
	asCStep:      function () { return proto.parseCStep(this.payload); },
	asTimeOfDay:  function () { return proto.parseTimeOfDay(this.payload); },
	asDate:       function () { return proto.parseDate(this.payload); }
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
			var msg = proto.parseRoutedWrite(packet);

			// The parser returns null, if the packet contents are invalid
			if (msg) {
				msg.__proto__ = MessagePrototype;
				callback(sender, msg);
			}
		});
	},

	send: function (src, dest, payload) {
		var buf = proto.makeRoutedWrite(src, dest, payload);
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
