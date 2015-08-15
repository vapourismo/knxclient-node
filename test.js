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
	message: function(ldata) {
		var ext = "";

		if (ldata.hasData()) {
			ext = ", value = " + ldata.extract(knxclient.dpt.Float16);
		}

		console.log("Message: " + ldata + ext);
	},
	disconnect: function(status) {
		console.log("Disconnect: status = " + status);
	}
});
