{
	"targets": [
		{
			"target_name": "knxproto",
			"sources": [
				"lib/knxproto/knxproto.cpp"
			],
			"cflags": [
				"-std=c++14",
				"-O2",
				"-Wall",
				"-Wextra",
				"-pedantic",
				"-fmessage-length=0",
				"-Wno-unused-parameter",
				"-Wno-missing-field-initializers",
				"-D_GLIBCXX_USE_C99",
				"-I../deps/jawra/lib"
			],
			"ldflags": [
				"-lknxproto"
			]
		}
	]
}
