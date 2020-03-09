#include "NLogger.hpp"

#define MODULE_OLD_TRANSCODER   "old-transcoder"
#define MODULE_YUV_MIX			"yuv-mix"

static NLogger::shared mlogger = NLogger::Get("main");

static
void print_usage(int argc, char* argv[]) {
	mlogger->info("usage:");
	mlogger->info("  {} [module] [args]", argv[0]);
	mlogger->info("modules:");
	mlogger->info("  {}", MODULE_OLD_TRANSCODER);
	mlogger->info("  {}", MODULE_YUV_MIX);
}

extern "C" {
	int old_transcoder_main(int argc, char* argv[]);
	int yuv_mix_main(int argc, char* argv[]);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		print_usage(argc, argv);
		return 0;
	}

	const std::string module_name = argv[1];
	if (module_name == MODULE_OLD_TRANSCODER) {
		return old_transcoder_main(argc - 1, argv + 1);
	}
	else if (module_name == MODULE_YUV_MIX) {
		return yuv_mix_main(argc - 1, argv + 1);
	}
	else {
		dbge(mlogger, "unknown module [{}]", module_name);
		print_usage(argc, argv);
	}

	return -1;
}