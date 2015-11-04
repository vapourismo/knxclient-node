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

function OutboundQueue(send, resend) {
	this.idle = true;
	this.outbound = [];

	this.send = send;
	this.resend = resend;
}

OutboundQueue.prototype.queue = function (msg) {
	this.outbound.unshift(msg);

	if (this.idle) {
		this.info = this.send(this.outbound[this.outbound.length - 1]);
		this.idle = false;
		this.resetInterval();
	}
};

OutboundQueue.prototype.confirm = function () {
	var item = this.outbound.pop();

	if (this.outbound.length == 0) {
		this.idle = true;
	} else {
		this.info = this.send(this.outbound[this.outbound.length - 1]);
		this.idle = false;
		this.resetInterval();
	}

	return item;
};

OutboundQueue.prototype.resetInterval = function () {
	if (this.interval)
		clearInterval(this.interval);

	this.interval = setInterval(function () {
		if (this.outbound.length > 0)
			this.resend(this.info, this.outbound[this.outbound.length - 1]);
	}.bind(this), 1000);
};

OutboundQueue.prototype.clearInterval = function () {
	if (this.interval)
		clearInterval(this.interval);

	this.interval = null;
};

function Tunnel(host, port) {
	EventEmitter.prototype.constructor.call(this);

	this.host = host || "localhost";
	this.port = port || 3671;

	this.outbound = new OutboundQueue(
		function (cemi) {
			if (this.ext) {
				var no = proto.sendTunnel(this.ext, cemi);
				return no;
			}
		}.bind(this),

		function (no, cemi) {
			if (this.ext && no != null) {
				proto.resendTunnel(this.ext, no, cemi);
			}
		}.bind(this)
	);

	this.ext = proto.createTunnel(
		function (state) {
			switch (state) {
				case 0:
					this.emit("connecting");
					break;

				case 1:
					this.emit("connected");
					break;

				case 2:
					this.emit("disconnecting");
					break;

				case 3:
					this.emit("disconnected");
					break;
			}
		}.bind(this),

		function (buf) {
			this.sock.send(buf, 0, buf.length, this.port, this.host);
		}.bind(this),

		function (msg) {
			if (!msg) return;

			if (msg.service == proto.LDataIndication)
				this.emit("indication", msg.payload.source, msg.payload.destination, msg.payload.tpdu, msg);
			else if (msg.service == proto.LDataConfirmation)
				this.emit("confirmation", msg.payload.source, msg.payload.destination, msg.payload.tpdu, msg);
		}.bind(this),

		function (no) {
			this.outbound.confirm();
		}.bind(this)
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
	return this.outbound.queue(cemi);
};

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
