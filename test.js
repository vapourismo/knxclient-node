var client = require("./src/knxclient.js")

var rt = new client.RouterClient(function () {
	this.send(1, 8, client.makeUnsigned8(8));
	this.send(1, 16, client.makeUnsigned16(16));
	this.send(1, 32, client.makeUnsigned32(32));
});

