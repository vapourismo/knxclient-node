var k = require("./src/knxclient.js")

var tunnel = new k.Tunnel("10.0.0.10", 3671);

tunnel.on("connected", function () {
	for (var i = 0; i < 256; i++) {
		var value = Math.round(Math.random() * 65535);
		var data = k.packUnsigned16(value);

		var cemi = {
			service: k.LDataRequest,
			payload: {
				source: 0,
				destination: i,
				tpdu: {
					tpci: k.UnnumberedData,
					apci: k.GroupValueWrite,
					payload: data
				}
			}
		};

		tunnel.send(cemi);
	}
});

tunnel.on("confirmation", function (s, d, t) {
	console.log("confirm", d);
});

tunnel.connect();
