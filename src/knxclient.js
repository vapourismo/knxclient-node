var dgram = require("dgram");

// TODO: Use 'bindings' package
var proto = require("../build/Release/knxclient_proto");

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

function RouterClient() {
	var sock = dgram.createSocket("udp4");

	sock.bind(3671, function() {
		this.addMembership("224.0.23.12");
		this.setMulticastLoopback(false);
	});

	this.sock = sock;
};

RouterClient.prototype = {
	listen: function(fn) {
		this.sock.on("message", function(packet, sender) {
			var ldata = proto.extractLData(packet);
			if (ldata) {
				ldata.__proto__ = LData.prototype;
				fn.call(this, sender, ldata);
			}
		});
	}
};

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

module.exports = {
	LData: LData,
	RouterClient: RouterClient,
	formatIndividualAddress: formatIndividualAddress,
	formatGroupAddress: formatGroupAddress
};
