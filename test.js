var knxclient = require("./src/knxclient.js");

var rc = new knxclient.RouterClient();

rc.listen(function(sender, ldata) {
	var tag = "[" + sender.address + ":" + sender.port + "] ";
	console.log(tag + ldata);
});
