var client = require("./src/knxclient.js")

var rt = new client.RouterClient();
rt.listen(function (sender, message) {
	console.log(message);

	switch (message.destination) {
		case 5:
			console.log("unsigned8(13)", message.asUnsigned8());
			break;

		case 7:
			console.log("unsigned16(37)", message.asUnsigned16());
			break;

		case 12:
			console.log("unsigned32(1337)", message.asUnsigned32());
			break;

		case 6:
			console.log("signed8(-13)", message.asSigned8());
			break;

		case 8:
			console.log("signed16(-37)", message.asSigned16());
			break;

		case 13:
			console.log("signed32(-1337)", message.asSigned32());
			break;

		case 9:
			console.log("float16(13.37)", message.asFloat16());
			break;

		case 14:
			console.log("float32(73.31)", message.asFloat32());
			break;

		case 1:
			console.log("bool(false)", message.asBool());
			break;

		case 2:
			console.log("cvalue(true, false)", message.asCValue());
			break;

		case 3:
			console.log("cstep(false, 2)", message.asCStep());
			break;

		case 10:
			console.log("timeofday(1, 2, 3, 4)", message.asTimeOfDay());
			break;

		case 11:
			console.log("date(4, 3, 2)", message.asDate());
			break;

	}
});
