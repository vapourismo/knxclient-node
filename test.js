var knxclient = require("./src/knxclient.js");

var tc = new knxclient.TunnelClient({
	host: "10.0.0.2",
	port: 3671,
	connected: function() {
		console.log("Connection: established, channel = " + this.channel);
	},
	error: function(code) {
		console.log("Connection: refused, code = " + code);
	},
	message: function(data) {
		console.log("Message: " + data);
	},
	disconnect: function(status) {
		console.log("Disconnect: status = " + status);
	}
});
