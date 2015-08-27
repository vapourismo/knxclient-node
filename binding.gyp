{
	"targets": [
		{
			"target_name": "knxclient_proto",
			"sources": [
				"lib/proto.cpp",
				"lib/builders.cpp",
				"lib/util.cpp"
			],
			"cflags": [
				"-std=c++11"
			],
			"ldflags": [
				"-lknxproto"
			]
		}
	]
}
