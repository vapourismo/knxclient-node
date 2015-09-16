var proto = require("./build/Release/knxproto.node");

var gw = proto.makeRoutedWrite(1337, 2563, new Buffer([0, 1]));
console.log(gw);

var res = proto.parseRoutedWrite(gw);
console.log(res);
