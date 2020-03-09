#include "NLogger.hpp"
#include "NVideoTranscoder.hpp"

#define __STDC_CONSTANT_MACROS

#define SDL_MAIN_HANDLED

extern "C" {
	#include "SDL.h"
	int old_transcoder_main(int argc, char* argv[]);
}

#define ENABLE_SDL 1
#define BGWIDTH 800
#define BGHEIGHT 800


#if ENABLE_SDL
static int SDLWIDTH = BGWIDTH;
static int SDLHEIGHT = BGHEIGHT;
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)

static int thread_exit = 0;

static int refresh_video(void* opaque) {
	thread_exit = 0;
	while (!thread_exit) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit = 0;
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}
#endif

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

	std::shared_ptr<nmedia::video::Transcoder> stc = nullptr;
	nmedia::video::Transcoder::OutputConfig config;
	std::vector<nmedia::video::Region::RegionConfig> reCfg;
	nmedia::video::Region::RegionConfig tmpcfg1;
	nmedia::video::Region::RegionConfig tmpcfg2;
	NVideoFrame* inFrame1 = nullptr;
	NVideoFrame* inFrame2 = nullptr;
	uint8_t* in_buf = nullptr;
	//uint8_t* in_buf = new uint8_t[1500];
	size_t packet_num = 0;

#if ENABLE_SDL
	SDL_Window* screen = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Rect rect;
	SDL_Thread* refreshThread = nullptr;
	SDL_Event event;
#endif

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

#if ENABLE_SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -10;
	}

	screen = SDL_CreateWindow("Simple Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SDLWIDTH, SDLHEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -11;
	}

	renderer = SDL_CreateRenderer(screen, -1, 0);
	if (!renderer) {
		printf("SDL : could not create renderer - exiting:&s\n", SDL_GetError());
		return -12;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		SDLWIDTH, SDLHEIGHT);

	rect.x = 0;
	rect.y = 0;
	rect.w = SDLWIDTH;
	rect.h = SDLHEIGHT;

	refreshThread = SDL_CreateThread(refresh_video, NULL, NULL);
#endif

	stc = std::make_shared<nmedia::video::Transcoder>("transcoder");


	//转码配置
	config.width = BGWIDTH;
	config.height = BGHEIGHT;
	config.backgroundColor = 0xffffff;
	config.framerate = 25;
	config.bitrate = 1800000;
	config.outCodecType = NCodec::Type::H264;




	//区域配置
	tmpcfg1 = { 1, 0, 0, 400, 400, 1, nmedia::video::Region::contentMode::ScalingModeAspectFit };
	tmpcfg2 = { 2, 400, 0, 400, 400, 1, nmedia::video::Region::contentMode::ScalingModeAspectFit };





	stc->init(config);
	stc->output([&out_file, &packet_num, &logger](const uint8_t* data, const size_t size) {
		if (out_file) {
			dbgi(logger, "write to file, size=[{}], packet num=[{}]. ", size, packet_num);
			fwrite(data, sizeof(uint8_t), size, out_file);
			++packet_num;
		}
	});

	reCfg.push_back(tmpcfg1);
	reCfg.push_back(tmpcfg2);
	stc->addRegions(reCfg);

	inFrame1 = new NVideoFrame();
	inFrame1->setSize({ 640,480 });
	inFrame1->setCodecType(NCodec::Type::H264, NMedia::Type::Video);

	// inFrame2 = new NVideoFrame();
	// inFrame2->setSize({ 640,480 });
	// inFrame2->setCodecType(NCodec::Type::H264, NMedia::Type::Video);

	while (1) {
#if ENABLE_SDL
		SDL_WaitEvent(&event);
		if (event.type = REFRESH_EVENT) {
#endif
			int ret = 0;

			int read_size = 1500;

			ret = fread(&read_size, sizeof(int), 1, in_file);
			if (!ret) {
				dbge(logger, "read infile error! ret=[{}]", ret);
				break;
			}

			in_buf = new uint8_t[read_size + AV_INPUT_BUFFER_PADDING_SIZE];
			memset(in_buf, 0x00, read_size + AV_INPUT_BUFFER_PADDING_SIZE);
			ret = fread(in_buf, sizeof(uint8_t), read_size, in_file);
			if (!ret) {
				dbge(logger, "read infile error! ret=[{}]", ret);
				break;
			}

			inFrame1->setData(in_buf, ret);

			stc->input(1, inFrame1);
			stc->input(2, inFrame1);


			stc->transcode();

			const AVFrame* frame_sws = stc->getDebug();
			if (!frame_sws) {
				continue;
			}

#if ENABLE_SDL
			SDL_UpdateYUVTexture(texture, &rect,
				frame_sws->data[0], frame_sws->linesize[0],
				frame_sws->data[1], frame_sws->linesize[1],
				frame_sws->data[2], frame_sws->linesize[2]);

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, &rect);
			SDL_RenderPresent(renderer);
		}
		else if (event.type == SDL_WINDOWEVENT) {
			SDL_GetWindowSize(screen, &SDLWIDTH, &SDLHEIGHT);
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
			goto END;
		}
		else if (event.type == BREAK_EVENT) {
			//ret = 0;
			break;
		}
#endif
		if (in_buf) {
			delete[] in_buf;
			in_buf = nullptr;
		}
	}

	//or stc->removeAllRegions();
	stc->removeRegion(tmpcfg1);
	stc->removeRegion(tmpcfg2);
	stc->close();

END:;

#if ENABLE_SDL
	SDL_Quit();
#endif

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

	if (inFrame2) {
		delete inFrame2;
		inFrame2 = nullptr;
	}

	return 0;
}
