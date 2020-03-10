#include "NLogger.hpp"
#include "NVideoTranscoder.hpp"
#include "NMediaFrame.hpp"
#include "SDLDisplay.hpp"

#define __STDC_CONSTANT_MACROS

#define SDL_MAIN_HANDLED

extern "C" {
	int old_transcoder_main(int argc, char* argv[]);
}

#define BGWIDTH 800
#define BGHEIGHT 800


int old_transcoder_main(int argc, char* argv[]) {
#ifdef _WIN32
	//std::string infile_str("C:/Users/Li/Desktop/640x480_output.h264p");
	std::string infile_str("C:/Users/Li/Desktop/output_640x480.h264p");
	std::string outfile_str("C:/Users/Li/Desktop/output.h264");
#else
    std::string infile_str("/tmp/output_640x480.h264p");
    std::string outfile_str("/tmp/output.h264");
#endif
	FILE* in_file = nullptr;
	FILE* out_file = nullptr;

	std::shared_ptr<nmedia::video::Transcoder> stc = nmedia::video::Transcoder::Create("h264p");
	nmedia::video::Transcoder::OutputConfig config;
	std::vector<nmedia::video::RegionConfig> reCfg;
	NVideoFrame* inFrame1 = nullptr;

	uint8_t* in_buf = nullptr;
	size_t packet_num = 0;
	NSDLDisplay::shared monitor = NSDLDisplay::Create("display");

	NLogger::EnableSinks("", true);
	NLogger::shared logger = NLogger::Get("main");

	in_file = fopen(infile_str.c_str(), "rb");
	if (!in_file) {
		dbge(logger, "failed to open out file, [{}]", infile_str);
		goto END;
	}

	out_file = fopen(outfile_str.c_str(), "wb");
	if (!out_file) {
		dbge(logger, "failed to open out file, [{}]", outfile_str);
		goto END;
	}

	//转码配置
	config.width = BGWIDTH;
	config.height = BGHEIGHT;
	config.backgroundColor = 0x0;
	config.framerate = 25;
	config.bitrate = 1800000;
	config.outCodecType = NCodec::Type::H264;

	stc->init(config);
	{
		//区域配置
		reCfg.push_back({ 0, 0, 0, 400, 800, 1, nmedia::video::ScalingMode::AspectFit });
		reCfg.push_back({ 1, 400, 0, 400, 800, 1, nmedia::video::ScalingMode::AspectFit });
		stc->setRegions(reCfg);
	}

	stc->output([&out_file, &packet_num, &logger](const uint8_t* data, const size_t size) {
		if (out_file) {
			dbgi(logger, "write to file, size=[{}], packet num=[{}]. ", size, packet_num);
			fwrite(data, sizeof(uint8_t), size, out_file);
			++packet_num;
		}
	});

	inFrame1 = new NVideoFrame();
	inFrame1->setSize({ 640,480 });
	inFrame1->setCodecType(NCodec::Type::H264, NMedia::Type::Video);

	monitor->OnFrame([&stc, &in_file, &logger, &in_buf, &inFrame1]()->const AVFrame* {
		int ret = 0;
		int read_size = 0;

		ret = fread(&read_size, sizeof(int), 1, in_file);
		if (!ret) {
			dbge(logger, "read infile error! ret=[{}]", ret);
			return nullptr;
		}

		in_buf = new uint8_t[read_size];
		memset(in_buf, 0x00, read_size);
		ret = fread(in_buf, sizeof(uint8_t), read_size, in_file);
		if (!ret) {
			dbge(logger, "read infile error! ret=[{}]", ret);
			return nullptr;
		}

		inFrame1->setData(in_buf, ret);
		if (in_buf) {
			delete[] in_buf;
			in_buf = nullptr;
		}

		stc->input(0, inFrame1);
		stc->input(1, inFrame1);

		const AVFrame* frame = nullptr;
		stc->transcode(&frame);

		return frame;
	});

	monitor->init(40, BGWIDTH, BGHEIGHT, NSDLDisplay::OutFmt::YUV420P);

	stc->close();

END:;
	if (in_file) {
		fclose(in_file);
		in_file = nullptr;
	}

	if (out_file) {
		fclose(out_file);
		out_file = nullptr;
	}

	if (inFrame1) {
		delete inFrame1;
		inFrame1 = nullptr;
	}

	return 0;
}
