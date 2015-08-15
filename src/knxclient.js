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

function LData() {}

LData.prototype = {
	hasData: function() {
		return (
			this.tpdu.tpci == proto.UnnumberedData ||
			this.tpdu.tpci == proto.NumberedData
		);
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

function TunnelClient(host, port) {
	var sock = dgram.createSocket("udp4");
	sock.on("message", this.onMessage.bind(this));

	var req = proto.makeConnectionRequest();
	sock.send(req, 0, req.length, port || 3671, host);

	this.sock = sock;
}

TunnelClient.prototype = {
	onMessage: function(packet, sender) {
		var knx = proto.parseKNX(packet);

		if (knx) {
			if (knx.service == proto.ConnectionResponse) {
				if (knx.status == 0) {
					this.channel = knx.channel;

					if (this.onConnect) {
						this.onConnect();
					}
				} else {
					this.sock.close();

					if (this.onError) {
						this.onError();
					}
				}
			} else if (knx.service == proto.DisconnectRequest && knx.channel == this.channel) {
				var res = proto.makeDisconnectResponse(this.channel);
				this.sock.send(req, 0, req.length,
				               sender.port, sender.address,
				               this.sock.close.bind(this.sock));

				if (this.onDisconnect) {
					this.onDisconnect(knx.status);
				}
			} else if (knx.service == proto.TunnelRequest && knx.channel == this.channel) {
				var res = proto.makeTunnelResponse(knx.channel, knx.seqNumber);
				this.sock.send(res, 0, res.length,
				               sender.port, sender.address);

				if (isLData(knx.data) && this.onData) {
					this.onData(knx.data.payload);
				}
			}
		}
	},
};

module.exports = {
	LData: LData,
	RouterClient: RouterClient,
	TunnelClient: TunnelClient,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress: formatGroupAddress
};
