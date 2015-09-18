var dgram = require("dgram");

// TODO: Use 'bindings' package
var proto = require("../build/Release/knxproto");

function formatGroupAddress(addr) {
	var a = (addr >> 11) & 15;
	var b = (addr >> 8) & 7
	var c = addr & 255;

	return a + "/" + b + "/" + c;
}

function formatIndividualAddress(addr) {
	var a = (addr >> 12) & 15;
	var b = (addr >> 8) & 15
	var c = addr & 255;

	return a + "." + b + "." + c;
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

function RouterClient(host, port) {
	if (port == null) {
		switch (typeof(host)) {
			case "string":
				port = 3671;
				break;

			case "number":
				port = host;
				host = "224.0.23.12";
				break;
		}
	}

	this.conf = {
		host: host || "224.0.23.12",
		port: port || 3671
	};

	this.sock = dgram.createSocket("udp4");
	this.sock.bind(this.conf.port, function () {
		this.sock.addMembership(this.conf.host);
		this.sock.setMulticastLoopback(false);
	}.bind(this));
}

RouterClient.prototype = {
	listen: function (callback) {
		this.sock.on("message", function (packet, sender) {
			var msg = proto.parseRoutedWrite(packet);
			if (msg) {
				msg.__proto__ = MessagePrototype;
				delete sender.size;

				callback(sender, msg);
			}
		});
	},

	write: function (src, dest, payload) {
		var buf = proto.makeRoutedWrite(src, dest, payload);
		this.sock.send(buf, 0, buf.length, this.conf.port, this.conf.address);
	}
};

module.exports = {
	RouterClient:            RouterClient,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress:      formatGroupAddress
};
