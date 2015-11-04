var k = require("./src/knxclient.js")

var router = new k.Router();

router.on("indication", function (src, dest, data) {
	var value = k.unpackUnsigned16(data.payload);
	console.log("router", value);

	router.write(0, dest, k.packUnsigned16(value * 2));
});

var tunnel = new k.Tunnel("10.0.0.10", 3671);

tunnel.on("state", function (state) {
	console.log("state", state);

	switch (state) {
		case 1:
			tunnel.write(0, 2563, k.packUnsigned16(42));
			break;

		case 3:
			tunnel.dispose();
			router.dispose();
			break;
	}
});

tunnel.on("indication", function (src, dest, data) {
	var value = k.unpackUnsigned16(data.payload);
	console.log("tunnel", value);

	tunnel.disconnect();
});

tunnel.connect();
