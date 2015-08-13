{
	"targets": [
		{
			"target_name": "knxclient_proto",
			"sources": [
				"lib/knxclient.cpp",
				"lib/util.cpp"
			],
			"cflags": [
				"-std=c++11"
			],
			"ldflags": [
				"-lknxclient"
			]
		}
	]
}
