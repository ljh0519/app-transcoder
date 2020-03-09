#ifndef __NSDLDISPLAY_HPP__
#define __NSDLDISPLAY_HPP__

#include <memory>
#include <functional>
#include <string>

extern "C" {
#include "libavutil\frame.h"
}

class NSDLDisplay {
public:
	using shared = std::shared_ptr< NSDLDisplay>;

	typedef std::function<const AVFrame * ()> onFrame;

	enum OutFmt {
		UNKOWN,
		YUV420P,
		RGB24
	};
public:
	NSDLDisplay() {}

	virtual ~NSDLDisplay() {}

	virtual int init(uint32_t delay
					, int width
					, int height
					, OutFmt out_fmt) = 0;

	virtual void OnFrame(const onFrame& func) = 0;

	static
	shared Create(const std::string& name);
};


#endif //__NSDLDISPLAY_HPP__