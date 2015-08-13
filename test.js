var knxclient = require("./build/Release/knxclient_proto");
var dgram = require("dgram");

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
			this.tpdu.tpci == UnnumberedData ||
			this.tpdu.tpci == NumberedData
		);
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
			var ldata = knxclient.extractLData(packet);
			if (ldata) {
				ldata.__proto__ = LData.prototype;
				fn.call(this, sender, ldata);
			}
		});
	}
};

var rc = new RouterClient();

rc.listen(function(sender, ldata) {
	var tag = "[" + sender.address + ":" + sender.port + "] ";
	var value = knxclient.parseAPDU(ldata.tpdu.payload, knxclient.Float16);

	console.log(tag + formatIndividualAddress(ldata.source) + " -> " + formatGroupAddress(ldata.destination) + ": " + value);
});
