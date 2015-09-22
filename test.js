var client = require("./src/knxclient.js")

var rt = new client.RouterClient();

rt.send(1, 8, client.makeUnsigned8(8));
rt.send(1, 16, client.makeUnsigned16(16));
rt.send(1, 32, client.makeUnsigned32(32));
