var client = require("./src/knxclient.js")

var rt = new client.RouterClient();
rt.listen(function (sender, message) {
	switch (message.destination) {
		case 5:
			var value = message.asUnsigned8();
			console.log(value, message.payload, client.makeUnsigned8(value));
			break;

		case 7:
			var value = message.asUnsigned16();
			console.log(value, message.payload, client.makeUnsigned16(value));
			break;

		case 12:
			var value = message.asUnsigned32();
			console.log(value, message.payload, client.makeUnsigned32(value));
			break;

		case 6:
			var value = message.asSigned8();
			console.log(value, message.payload, client.makeSigned8(value));
			break;

		case 8:
			var value = message.asSigned16();
			console.log(value, message.payload, client.makeSigned16(value));
			break;

		case 13:
			var value = message.asSigned32();
			console.log(value, message.payload, client.makeSigned32(value));
			break;

		case 9:
			var value = message.asFloat16();
			console.log(value, message.payload, client.makeFloat16(value));
			break;

		case 14:
			var value = message.asFloat32();
			console.log(value, message.payload, client.makeFloat32(value));
			break;

		case 1:
			var value = message.asBool();
			console.log(value, message.payload, client.makeBool(value));
			break;

		case 2:
			var value = message.asCValue();
			console.log(value, message.payload, client.makeCValue(value));
			break;

		case 3:
			var value = message.asCStep();
			console.log(value, message.payload, client.makeCStep(value));
			break;

		case 10:
			var value = message.asTimeOfDay();
			console.log(value, message.payload, client.makeTimeOfDay(value));
			break;

		case 11:
			var value = message.asDate();
			console.log(value, message.payload, client.makeDate(value));
			break;

	}
});
