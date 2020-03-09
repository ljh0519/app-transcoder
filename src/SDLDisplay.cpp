#include "SDLDisplay.hpp"


extern "C" {
#include "SDL.h"
}


class NSDLDisplayImpl : public NSDLDisplay{
public:
	using ThizClass = NSDLDisplayImpl;
	using shared = std::shared_ptr< NSDLDisplayImpl>;

	//typedef std::function<AVFrame * ()> onFrame;
	//typedef onFrame onYUVFrame;
	//typedef onFrame onRGBFrame;
	static const int REFRESH_EVENT = (SDL_USEREVENT + 1);
	static const int BREAK_EVENT = (SDL_USEREVENT + 2);
private:
	int thread_exit_ = 0;
	int delay_ = 0;
	int width_ = 0;
	int height_ = 0;
	OutFmt fmt_ = OutFmt::UNKOWN;
	onFrame input_ = nullptr;
	SDL_Window* screen = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Rect rect;
	//SDL_Thread* refreshThread = nullptr;

public:
	NSDLDisplayImpl(const std::string& name) {}

	virtual ~NSDLDisplayImpl() {
		SDL_Quit();
	}

	virtual int init(uint32_t delay
					, int width
					, int height
					, OutFmt out_fmt) override {
			
		delay_ = delay;
		width_ = width;
		height_ = height;
		fmt_ = out_fmt;

		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
			printf("Could not initialize SDL - %s\n", SDL_GetError());
			return -10;
		}

		screen = SDL_CreateWindow("Simple Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		if (!screen) {
			printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
			return -11;
		}

		renderer = SDL_CreateRenderer(screen, -1, 0);
		if (!renderer) {
			printf("SDL : could not create renderer - exiting:&s\n", SDL_GetError());
			return -12;
		}

		texture = SDL_CreateTexture(renderer, convertFmt(out_fmt), SDL_TEXTUREACCESS_STREAMING,
			width, height);

		rect.x = 0;
		rect.y = 0;
		rect.w = width;
		rect.h = height;

		SDL_CreateThread(refresh_video, NULL, this);

		return main_loop();
	}

	virtual void OnFrame(const onFrame& func) override {
		input_ = func;
	}

private:
	
	static int refresh_video(void* opaque) {
		auto thiz = reinterpret_cast<ThizClass*>(opaque);
		thiz->thread_exit_ = 0;
		while (!thiz->thread_exit_) {
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
			SDL_Delay(thiz->delay_);
		}
		thiz->thread_exit_ = 0;
		SDL_Event event;
		event.type = BREAK_EVENT;
		SDL_PushEvent(&event);

		return 0;
	}

	inline int main_loop() {
		if (!input_) {
			printf("input function is null!");
			return -1;
		}

		SDL_Event event;

		while (1) {
			SDL_WaitEvent(&event);
			if (event.type = REFRESH_EVENT) {
				const AVFrame* outFrame = input_();
				if (!outFrame) {
					break;
				}
				if (fmt_ == OutFmt::YUV420P) {
					handleYUVFrame(outFrame);
				}
				else if (fmt_ == OutFmt::RGB24) {
					handleRGBFrame(outFrame);
				}

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, &rect);
				SDL_RenderPresent(renderer);
			}
			else if (event.type == SDL_WINDOWEVENT) {
				SDL_GetWindowSize(screen, &width_, &height_);
			}
			else if (event.type == SDL_QUIT) {
				thread_exit_ = 1;
				break;
			}
			else if (event.type == BREAK_EVENT) {
				//ret = 0;
				break;
			}
		}

		return 0;
	}

	inline int convertFmt(OutFmt fmt) {
		if (fmt == OutFmt::YUV420P
			|| fmt == OutFmt::UNKOWN) {
			return SDL_PIXELFORMAT_IYUV;
		}
		else if(fmt == OutFmt::RGB24){
			return SDL_PIXELFORMAT_BGR24;
		}
	}

	inline void handleYUVFrame(const AVFrame* frame) {
		SDL_UpdateYUVTexture(texture, &rect,
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]);
	}

	inline void handleRGBFrame(const AVFrame* frame) {
		SDL_UpdateTexture(texture, &rect,
			frame->data[0], frame->linesize[0]);
	}
};

NSDLDisplay::shared NSDLDisplay::Create(const std::string& name) {
	return std::make_shared< NSDLDisplayImpl>(name);
}