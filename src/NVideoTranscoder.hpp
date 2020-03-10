#ifndef NVideoTranscoder_hpp
#define NVideoTranscoder_hpp

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "NRegion.hpp"
#include "NMediaBasic.hpp"
#include "NMediaFrame.hpp"

#include "fmt/fmt.h"

extern "C" {
#include "libavutil/frame.h"
}

namespace nmedia {
	namespace video {

		//TODO : 目前已知问题，在转换视频分辨率和关闭视频流时会丢失几帧视频帧
		class Transcoder {
		public:
			//转码器的配置参数
			struct OutputConfig {
				int width = -1;
				int height = -1;
				int backgroundColor = 0xffffff;
				int framerate = -1;
				int bitrate = -1;
				NCodec::Type outCodecType = NCodec::Type::UNKNOWN;

				bool vaild() const {
					return (0 < width)
						&& (0 < height)
						&& (0 < framerate)
						&& (0 < bitrate)
						&& (NCodec::Type::UNKNOWN < outCodecType);
				}

				const std::string dump() const {
					return fmt::format("[width=[{}]\nheight=[{}]\nbackgroundColor=[{x:}]\nframerate=[{}]\nbitrate=[{}]\noutCodecType=[{}].]"
						, width
						, height
						, backgroundColor
						, framerate
						, bitrate
						, outCodecType);
				}
			};
		public:
			using shared = std::shared_ptr< Transcoder>;

			using DataFunc = std::function<void(uint8_t * data, size_t size)>;

		public:
			Transcoder() {}

			virtual ~Transcoder() {}

			//设置转码器输出数据时的回调函数
			virtual void output(const DataFunc& func) = 0;

			//初始化转码器
			// 0 : 成功
			// ALREADY_OPENED_TRANSCODER : 转码器已初始化
			// EXTERNAL_PARAM_NOT_VAILD : 输入cfg参数不可用
			// FAILED_INIT_YUVMIXER : yuv mixer初始化失败
			// FAILED_INIT_ENCODER : 初始化编码器失败
			virtual int init(const OutputConfig& cfg) = 0;

			//获取转码器参数
			virtual const OutputConfig& getOutConfig() const = 0;

			//增加一路或多路region，index不能重复，如果有重复，则缩放图都无法加入，需重新调用方法"
			//逻辑：setRegions可以增加单一或者多路区域
			//当增加的区域编号为已有编号时，增加失败并返回冲突编号，不会增加任何编号区域
			//@channels index必须从0开始递增
			// 0 : 成功
			// region.index : index索引的参数不可用
			// PARAM_EXISTS : 传入的参数包含重复的index
			virtual int setRegions(const std::vector<RegionConfig>& channels) = 0;

			// 增加一路region
			// EXTERNAL_PARAM_NOT_VAILD : region参数不可用
			// PARAM_EXISTS : region存在
			// region.index : 成功
			virtual int addRegion(const RegionConfig& channel) = 0;

			// 当有数据时调用该方法输入数据
			// 0 : 成功
			// NOT_SUPPORT_CODEC_TYPE : 不支持输入视频流的格式
			// PARAM_NOT_EXISTS : index不存在
			virtual int input(int regionIndex, NVideoFrame* pkt) = 0;

			//转码并异步输出视频流
			//转码的视频流参数由初始化转码器时传入的参数决定
			//调用transcode将编码的帧通过回调函数输出
			// 0 : 成功
			virtual int transcode(const AVFrame** frame) = 0;

			//转码器是否开启
			virtual bool isOpened() const = 0;

			//关闭转码器
			virtual void close() = 0;

			//是否支持传入的编码类型
			virtual bool isSupportCodecType(const NCodec::Type typ) const = 0;

			//创建一个Transcoder实例
			static
			shared Create(const std::string& name);
		};
	}
}

#endif //NVideoTranscoder_hpp
