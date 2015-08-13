var knxclient = require("./build/Release/knxclient");
var dgram = require("dgram");

var RouterClient = function() {
	var sock = dgram.createSocket("udp4");

	sock.bind(3671, function() {
		this.addMembership("224.0.23.12");
		this.setMulticastLoopback(false);
	});

	this.sock = sock;
};

RouterClient.prototype.listen = function(fn) {
	this.sock.on("message", function(packet, sender) {
		var cnts = knxclient.parseKNX(packet);

		if (cnts) {
			fn.call(this, sender, cnts);
		}
	});
};

var rc = new RouterClient();

rc.listen(function(sender, payload) {
	console.log(sender);
	console.log(payload);
});
