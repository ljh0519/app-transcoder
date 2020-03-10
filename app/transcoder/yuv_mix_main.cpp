#include "NLogger.hpp"
#include "YUVMixer.hpp"

#include "SDLDisplay.hpp"

extern "C" {
#include "libavutil/imgutils.h"
	int yuv_mix_main(int argc, char* argv[]);
}

#define BGWIDTH 800
#define BGHEIGHT 800

class yuvReader {
public:
	using shared = std::shared_ptr< yuvReader>;
private:
	NLogger::shared logger_ = nullptr;
	FILE* file_ = nullptr;
	int width_ = 0;
	int height_ = 0;
	uint8_t* buf_ = 0;
	std::string file_path_;
	bool	cycle_ = false;

public:
	static shared Create(const std::string& name) {
		return std::make_shared< yuvReader>(name);
	}

	yuvReader(const std::string& name) :logger_(NLogger::Get(name)) {}

	~yuvReader() {
		if (file_) {
			fclose(file_);
			file_ = nullptr;
		}

		if (buf_) {
			delete[] buf_;
			buf_ = nullptr;
		}
	}

	int open(const std::string& file_path
			,int width
			,int height
			,bool cycle) {
		if (isOpened()) {
			return -1;
		}

		if (file_path.empty()
		||	width < 1
		||	height < 1) {
			return -2;
		}

		file_ = fopen(file_path.c_str(), "rb");
		if (!file_) {
			dbge(logger_, "failed to open file : [{}].", file_path);
			return -3;
		}

		width_ = width;
		height_ = height;
		file_path_ = file_path;
		cycle_ = cycle;

		buf_ = new uint8_t[width_ * height_ * 3 / 2];

		return 0;
	}

	bool isOpened() {
		return nullptr != file_;
	}

	int read(AVFrame* input) {
		if (!isOpened()) {
			dbge(logger_, "file is not open: [{}].", file_path_);
			return -1;
		}

		if (!input) {
			dbge(logger_, "input frame is null");
			return -2;
		}

		int ret = fread(buf_, sizeof(uint8_t), width_ * height_ * 3 / 2, file_);
		if (ret <= 0) {
			if (0 != ret || !cycle_) {
				dbge(logger_, "Error reading bytes from file : [{}].", file_path_);
				return -3;
			}

			fseek(file_, SEEK_SET, 0);
			ret = fread(buf_, sizeof(uint8_t), width_ * height_ * 3 / 2, file_);
			if (ret <= 0) {
				dbge(logger_, "Error reading bytes from file : [{}].", file_path_);
				return -3;
			}
		}

		if (av_image_fill_arrays(input->data, input->linesize
			, buf_, AV_PIX_FMT_YUV420P
			, width_
			, height_, 1) < 0) {
			dbge(logger_, "Could not init input buffer!");
			return -4;
		}

		input->width = width_;
		input->height = height_;
		input->format = AV_PIX_FMT_YUV420P;

		return ret;
	}
};

int yuv_mix_main(int argc, char* argv[]) {
	NLogger::shared logger = NLogger::Get("main");

	yuvReader::shared reader1 = yuvReader::Create("111");
	yuvReader::shared reader2 = yuvReader::Create("222");

	if (reader1->open("C:/Users/Li/Desktop/ds_480x272_yuv420p.yuv", 480, 272, true) < 0) {
		dbge(logger, "failed to open input file!");
		return -1;
	}

	if (reader2->open("C:/Users/Li/Desktop/out_640x480_yuv420p.yuv", 640, 480, true) < 0) {
		dbge(logger, "failed to open input file!");
		return -2;
	}

	nmedia::video::YUVMixer::shared mixer = nmedia::video::YUVMixer::Create("1&2");

	if (mixer->outputConfig(BGWIDTH, BGHEIGHT, 0) < 0) {
		dbge(logger, "failed to set output config!");
		return -3;
	}

	//----------------------------------------------------
	//check addRegion
	//if (mixer->addRegion({ 0, 0 ,0 , 400, 800, 1, nmedia::video::ScalingMode::AspectFit }) < 0) {
	//	dbge(logger, "failed to set output config!");
	//	return -4;
	//}
	//if (mixer->addRegion({ 1, 400 ,0 , 400, 800, 1, nmedia::video::ScalingMode::AspectFit }) < 0) {
	//	dbge(logger, "failed to set output config!");
	//	return -5;
	//}

	//check setRegions
	{
		std::vector<nmedia::video::RegionConfig> v;
		v.push_back({ 0, 0 , 0, 400, 800, 1, nmedia::video::ScalingMode::Fill });
		v.push_back({ 1, 400 , 0 , 400, 800, 1, nmedia::video::ScalingMode::AspectFit });

		if (mixer->setRegions(v) < 0) {
			dbge(logger, "failed to set output config!");
			return -6;
		}
	}

	AVFrame* inputFrame = av_frame_alloc();
	const AVFrame* outputFrame = nullptr;
	//----------------------------------------------------
	
	NSDLDisplay::shared sdl_dis = NSDLDisplay::Create("sdl");
	sdl_dis->OnFrame([&reader1, &inputFrame, &reader2, &mixer]()->const AVFrame* {
		if (reader1->read(inputFrame) < 0) {
			return nullptr;
		}
		if (mixer->inputRegionFrame(0, inputFrame) < 0) {
			return nullptr; 
		}

		if (reader2->read(inputFrame) < 0) {
			return nullptr;
		}
		if (mixer->inputRegionFrame(1, inputFrame) < 0) {
			return nullptr;
		}

		return mixer->outputFrame();
	});

	sdl_dis->init(40, BGWIDTH, BGHEIGHT, NSDLDisplay::OutFmt::YUV420P);
	//sdl_dis->init(40, 705, 400, NSDLDisplay::OutFmt::YUV420P);

	if (inputFrame) {
		av_frame_free(&inputFrame);
	}

	return 0;
}