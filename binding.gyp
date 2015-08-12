{
	"targets": [
		{
			"target_name": "knxclient",
			"sources": [
				"lib/knxclient.cpp",
				"lib/knx.cpp"
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
