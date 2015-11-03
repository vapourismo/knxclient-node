var k = require("./src/knxclient.js")

var router = new k.Router();

router.on("message", function (msg) {
	router.write(0, msg.destination, k.Unsigned16(msg.asUnsigned16() * 2));
});

var tunnel = new k.Tunnel("10.0.0.7", 3671);

tunnel.on("state", function (state) {
	switch (state) {
		case 1:
			tunnel.write(0, 2563, k.Unsigned16(42), false);
			break;

		case 3:
			tunnel.dispose();
			router.dispose();
			break;
	}
});

tunnel.on("message", function (msg) {
	if (msg.service != k.LDataIndication) return;
	tunnel.disconnect();
});

tunnel.connect();
