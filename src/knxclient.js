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

RouterClient.prototype = {
	listen: function(fn) {
		this.sock.on("message", this.onMessage.bind(this, fn || function() {}));
	},

	onMessage: function(fn, packet, sender) {
		var ldata = proto.extractLData(packet);

		if (ldata) {
			ldata.__proto__ = LData.prototype;
			fn.call(this, sender, ldata);
		}
	}
};

function TunnelClient(host, port, connectionHandler) {
	if (port != null) {
		if (typeof(port) == "function") {
			connectionHandler = port;
			port = 3671;
		}
	}

	var sock = dgram.createSocket("udp4");
	sock.on("message", this.onMessageConnecting.bind(this));

	var req = proto.makeConnectionRequest();
	sock.send(req, 0, req.length, port, host);

	this.onConnect = connectionHandler || function() {};
	this.sock = sock;
}

TunnelClient.prototype = {
	onMessageConnecting: function(packet, sender) {
		var knx = proto.parseKNX(packet);

		if (knx && knx.service == proto.ConnectionResponse) {
			if (knx.status == 0) {
				this.channel = knx.channel;
				this.sock.on("message", this.onMessage.bind(this));

				this.onConnect();
			} else {
				this.sock.close();
			}
		}
	},

	onMessage: function(packet, sender) {
		var knx = proto.parseKNX(packet);

		if (knx) {
			if (knx.service == proto.DisconnectRequest) {
				if (knx.channel == this.channel) {

				}
			}
		}
	}
};

module.exports = {
	LData: LData,
	RouterClient: RouterClient,
	TunnelClient: TunnelClient,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress: formatGroupAddress
};
