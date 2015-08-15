var dgram = require("dgram");

// TODO: Use 'bindings' package
var proto = require("../build/Release/knxclient_proto");

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

dgram.Socket.prototype.sendDatagram = function(address, port, buf, handler) {
	return this.send(buf, 0, buf.length, port, address, handler);
};

function LData() {}

LData.prototype = {
	hasData: function() {
		return (
			this.tpdu.tpci == proto.UnnumberedData ||
			this.tpdu.tpci == proto.NumberedData
		);
	},

	extract: function(dpt) {
		if (this.hasData())
			return proto.parseAPDU(this.tpdu.payload, dpt);
		else
	    	return null;
	},

	toString: function() {
		var ret = formatIndividualAddress(this.source) + " -> ";

		if (this.addressType == proto.GroupAddress)
			ret += formatGroupAddress(this.destination);
		else
			ret += formatIndividualAddress(this.destination);

		if (this.hasData()) {
			ret += ": apci = " + this.tpdu.apci + ", data =";
			for (var i = 0; i < this.tpdu.payload.length; i++)
				ret += " " + this.tpdu.payload[i];
		} else {
			ret += ": control = " + this.tpdu.control;
		}

		return ret;
	}
};

function RouterClient(host, port) {
	if (host != null && port == null && typeof(host) == "number") {
		port = host;
		host = null;
	}

	var sock = dgram.createSocket("udp4");

	sock.bind(port || 3671, function() {
		this.addMembership(host || "224.0.23.12");
		this.setMulticastLoopback(false);
	});

	this.sock = sock;
}

function isLData(cemi) {
	return (
		cemi.service == proto.LDataReq ||
		cemi.service == proto.LDataInd ||
		cemi.service == proto.LDataCon
	);
}

RouterClient.prototype = {
	listen: function(fn) {
		this.sock.on("message", this.onMessage.bind(this, fn || function() {}));
	},

	onMessage: function(fn, packet, sender) {
		var knx = proto.parseKNX(packet);

		if (knx && knx.service == proto.RoutingIndication && isLData(knx.data)) {
			var ldata = knx.data.payload;
			ldata.__proto__ = LData.prototype;
			fn.call(this, ldata);
		}
	}
};

function TunnelClient(conf) {
	this.conf = {
		host:       conf.host,
		port:       conf.port || 3671,
		timeout:    conf.timeout || 5000,
		connected:  conf.connected || function() {},
		error:      conf.error || function() {},
		message:    conf.message || function() {},
		disconnect: conf.disconnect || function() {}
	};

	this.sock = dgram.createSocket("udp4");
	this.sock.on("message", TunnelClient.processMessage.bind(this));
	this.sock.sendDatagram(this.conf.host, this.conf.port, proto.makeConnectionRequest());

	this.connectionTimeout = setTimeout(TunnelClient.processTimeout.bind(this), this.conf.timeout);
}

TunnelClient.ErrorCode = {
	Timeout: 1,
	Refused: 2
};

TunnelClient.processMessage = function(packet, sender) {
	var knx = proto.parseKNX(packet);

	if (knx) {
		if (knx.service == proto.ConnectionResponse) {
			if (knx.status == 0) {
				clearTimeout(this.connectionTimeout);
				this.connectionTimeout = null;

				this.channel = knx.channel;
				this.conf.connected.call(this);
			} else {
				this.sock.close();
				this.conf.error.call(this, TunnelClient.ErrorCode.Refused);
			}
		} else if (knx.service == proto.DisconnectRequest && knx.channel == this.channel) {
			this.sock.sendDatagram(sender.address, sender.port,
			                       proto.makeDisconnectResponse(this.channel),
			                       this.sock.close.bind(this.sock));
			this.channel = null;
			this.conf.disconnect.call(this, knx.status);
		} else if (knx.service == proto.TunnelRequest && knx.channel == this.channel) {
			this.sock.sendDatagram(sender.address, sender.port,
			                       proto.makeTunnelResponse(knx.channel, knx.seqNumber));

			if (isLData(knx.data)) {
				var ldata = knx.data.payload;
				ldata.__proto__ = LData.prototype;
				this.conf.message.call(this, ldata);
			}
		}
	}
};

TunnelClient.processTimeout = function() {
	if (!this.channel) {
		this.sock.close();
		this.conf.error.call(this, TunnelClient.ErrorCode.Timeout);
	}

	this.connectionTimeout = null;
};

TunnelClient.prototype = {
	disconnect: function(reason) {
		reason = reason || 0;

		if (this.channel) {
			// TODO: Make disconnect request
			// TODO: Send disconnect request
		}
	}
};

module.exports = {
	LData: LData,
	RouterClient: RouterClient,
	TunnelClient: TunnelClient,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress: formatGroupAddress,
	dpt: proto.dpt
};
