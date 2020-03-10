
#include <utility>
#include <algorithm>
#include <map>

#include "NVideoTranscoder.hpp"
#include "NMediaFrame.hpp"
#include "NLogger.hpp"
#include "YUVMixer.hpp"
#include "NTErrorDefined.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
};



namespace nmedia {
	namespace video {

		static 
		inline int convertFFCodecID(NCodec::Type typ) {
			if (NCodec::Type::VP8 == typ) {
				return AV_CODEC_ID_VP8;
			}
			else if (NCodec::Type::H264 == typ) {
				return AV_CODEC_ID_H264;
			}
			return NOT_SUPPORT_CODEC_TYPE;
		}

		static const int OUT_FF_FMT = AV_PIX_FMT_YUV420P;

		class Region {
		public:

		private:
			NLogger::shared         logger_;
			RegionConfig            RgConfig_;
			AVPacket				*imgPacket_ = nullptr;
			AVFrame					*inFrame_ = nullptr;
			AVCodecParserContext	*imgParserCtx_ = nullptr;
			AVCodecContext			*imgCodecCtx_ = nullptr;

		public:
			using shared = std::shared_ptr<Region>;

			Region(NLogger::shared logger) :logger_(logger) { }

			Region(const std::string& name) :logger_(NLogger::Get(name)) { }

			virtual ~Region() {
				close();
			}

			//初始化改区域
			int init(const RegionConfig& config) {
				if (isOpened()) {
					return ALREADY_OPENED_REGION;
				}

				if (!config.valid()) {
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				RgConfig_ = config;

				return 0;
			}

			//当有数据输入时，该区域应该调用这个方法
			//这里传入的数据应该携带分辨率信息
			int onInputFrame(NVideoFrame* inPacket) {
				if (!imgCodecCtx_) {
					if (initDecoder(inPacket->getCodecType()) < 0) {
						return FAILED_INIT_DECODER;
					}

					if (initImgParser(inPacket->getCodecType()) < 0) {
						return FAILED_INIT_PARSER;
					}
				}

				return decoder(inPacket);
			}

			//获取区域中流被解码后的图像
			const AVFrame* getDrawFrame() const {
				return inFrame_;
			}

			//获取区域的配置信息
			const RegionConfig& getRegionCfg() const {
				return RgConfig_;
			}

			bool isOpened() const {
				return imgCodecCtx_ && imgParserCtx_;
			}

			//刷新区域的解码器
			void flushDecoder() const {
				if (!isOpened()) {
					return;
				}

				imgPacket_->data = nullptr;
				imgPacket_->size = 0;

				int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						avcodec_receive_frame(imgCodecCtx_, inFrame_);
						av_frame_unref(inFrame_);
						avcodec_send_packet(imgCodecCtx_, imgPacket_);
					}
					else {
						dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", RgConfig_.index, ret);
						return;
					}
				}

				ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						return;
					}
					char arr[1024] = { 0 };
					av_strerror(ret, arr, 1024);
					dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", RgConfig_.index, arr);
					return;
				}

			}

			//关闭区域
			void close() {
				flushDecoder();

				if (imgPacket_) {
					av_packet_unref(imgPacket_);
					imgPacket_ = nullptr;
				}

				if (inFrame_) {
					av_frame_free(&inFrame_);
				}

				if (imgParserCtx_) {
					av_parser_close(imgParserCtx_);
					imgParserCtx_ = nullptr;
				}

				if (imgCodecCtx_) {
					avcodec_close(imgCodecCtx_);
					imgCodecCtx_ = nullptr;
				}
			}

			const AVFrame* getSrcFrame() {
				return inFrame_;
			}

		private:
			//进行转码，传入的视频帧可以不携带分辨率信息，因为嗅探器会自动嗅探视频帧分辨率
			//size表示视频帧应该被缩放的大小，用于刷新缩放模块
			//这里将缩放模块放在这里。主要是因为嗅探器可以获取视频帧的帧格式，缩放模块的初始化需要这个帧格式。
			inline int decoder(NVideoFrame* pkt) {
				if (!isOpened()) {
					dbge(logger_, "transcoder is not open! index=[{}].", RgConfig_.index);
					return NOT_OPENED_REGION;
				}

				imgPacket_->data = pkt->data();
				imgPacket_->size = pkt->size();

				int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						avcodec_receive_frame(imgCodecCtx_, inFrame_);
						av_frame_unref(inFrame_);
						avcodec_send_packet(imgCodecCtx_, imgPacket_);
					}
					else {
						dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", RgConfig_.index, ret);
						return ERROR_DECODE_VIDEO;
					}
				}

				ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						return 0;
					}
					dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", RgConfig_.index, ret);
					return ERROR_DECODE_VIDEO;
				}

				av_packet_unref(imgPacket_);
				return 0;
			}

			inline int decoder1(NVideoFrame* pkt, const NVideoSize& size) {
				if (!isOpened()) {
					dbge(logger_, "transcoder is not open! index=[{}].", RgConfig_.index);
					return NOT_OPENED_REGION;
				}

				int frameSize = pkt->size();
				const uint8_t* pBuf = pkt->data();
				int ret = 0;

				while (frameSize) {

					int parse_len = av_parser_parse2(imgParserCtx_, imgCodecCtx_
						, &imgPacket_->data, &imgPacket_->size
						, pBuf, frameSize
						, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

					pBuf += parse_len;
					frameSize -= parse_len;

					if (imgPacket_->size) {

						int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
						if (ret) {
							if (AVERROR(EAGAIN) == ret) {
								avcodec_receive_frame(imgCodecCtx_, inFrame_);
								av_frame_unref(inFrame_);
								avcodec_send_packet(imgCodecCtx_, imgPacket_);
							}
							else {
								dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", RgConfig_.index, ret);
								return ERROR_DECODE_VIDEO;
							}
						}

						ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
						if (ret) {
							if (AVERROR(EAGAIN) == ret) {
								return 0;
							}
							dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", RgConfig_.index, ret);
							return ERROR_DECODE_VIDEO;
						}

						av_packet_unref(imgPacket_);
					}
				}

				av_packet_unref(imgPacket_);
				return 0;
			}

			//初始化解码器
			inline int initDecoder(const NCodec::Type typ) {
				if (!imgCodecCtx_) {
					int codecId = convertFFCodecID(typ);
					if (codecId <= 0) {
						dbge(logger_, "Unsupported decoder type!  index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return codecId;
					}

					AVCodec* pCodec = avcodec_find_decoder((AVCodecID)codecId);
					if (!pCodec) {
						dbge(logger_, "Can not find decoder! index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return NOT_SUPPORT_CODEC_TYPE;
					}

					imgCodecCtx_ = avcodec_alloc_context3(pCodec);
					if (!imgCodecCtx_) {
						dbge(logger_, "Could not allocate video codec context! index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return FAILED_INIT_DECODER;
					}

					if (avcodec_open2(imgCodecCtx_, pCodec, NULL) < 0) {
						dbge(logger_, "Could not open codec! index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return FAILED_INIT_DECODER;
					}
				}

				if (!inFrame_) {
					inFrame_ = av_frame_alloc();
				}

				return 0;
			}

			//初始化图像嗅探器，用于从网络流中识别一整帧可用视频帧
			inline int initImgParser(const NCodec::Type typ) {
				if (!imgParserCtx_) {
					int codecId = convertFFCodecID(typ);
					if (codecId <= 0) {
						dbge(logger_, "Unsupported decoder type!  index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return codecId;
					}

					imgParserCtx_ = av_parser_init(codecId);
					//avParserContext->flags |= PARSER_FLAG_ONCE;
					if (!imgParserCtx_) {
						dbge(logger_, "Could not init avParserContext! index=[{}], NCodec::Type=[{}].", RgConfig_.index, typ);
						return FAILED_INIT_PARSER;
					}
				}

				if (!imgPacket_) {
					imgPacket_ = av_packet_alloc();
				}

				return 0;
			}

		};

		//画布格式为YUV420P
		//占用内存计算：width * height * (3 / 2)
		class TranscoderImpl : public Transcoder {
		private:
			NLogger::shared             logger_ = nullptr;

			//需求：
			//RegionConfig中index作为增删改查的索引，zOrder作为绘图时的排序key。
			//channels作为绘图时用，以zOrder作为key排序
			//numbers_作为增删改查时用，以index作为增删改查的索引
			std::vector<Region::shared> channels_;
			std::map<int, Region::shared>   numbers_;

			YUVMixer::shared			yuvMixer_ = nullptr;
			Transcoder::OutputConfig	cfg_;
			//av_new_packet()   //申请avpacket的buffer空间。
			//av_packet_alloc();
			AVPacket*					outPacaket_ = nullptr;
			AVCodecContext*				imgCodecCtx_ = nullptr;
			Transcoder::DataFunc        onEncodeFrame_ = nullptr;

		public:
			TranscoderImpl(NLogger::shared logger) :logger_(logger) {}

			TranscoderImpl(const std::string& name) :logger_(NLogger::Get(name)) {}

			virtual ~TranscoderImpl() {
				close();
			}

			//设置转码器输出数据时的回调函数
			virtual void output(const Transcoder::DataFunc& func) override {
				onEncodeFrame_ = func;
			}

			//初始化转码器
			virtual int init(const Transcoder::OutputConfig& cfg) override {
				if (isOpened()) {
					dbge(logger_, "transcoder already opened! cfg=[{}].", cfg_.dump());
					return ALREADY_OPENED_TRANSCODER;
				}

				if (!cfg.vaild()) {
					dbge(logger_, "transcoder init param error! cfg=[{}].", cfg.dump());
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				av_log_set_level(AV_LOG_QUIET);

				int ret = 0;

				cfg_ = cfg;

				yuvMixer_ = YUVMixer::Create("yuv_mix");
				ret = yuvMixer_->outputConfig(cfg.width, cfg.height, cfg.backgroundColor);
				if (ret) {
					dbge(logger_, "Initializing the yuv mixer failed! error=[{}].", ret);
					return FAILED_INIT_YUVMIXER;
				}

				ret = initCodec();
				if (ret) {
					return FAILED_INIT_ENCODER;
				}

				return 0;
			}

			//获取转码器参数
			virtual const Transcoder::OutputConfig& getOutConfig() const override {
				return cfg_;
			}

			//增加一路或多路图，index不能重复，如果有重复，则缩放图都无法加入，需重新调用方法
			//逻辑：addRegions可以增加单一或者多路区域
			//当增加的区域编号为已有编号时，增加失败并返回冲突编号，不会增加任何编号区域
			//@channels index must increase from 1。
			//@return 0 on success, otherwise negative error code or error number
			// -1 : region.index重复
			// region.index : region参数不可用
			// -2 : yuv mixer中region增加失败
			virtual int setRegions(const std::vector<RegionConfig>& channels) override {
				{
					std::map<int, std::shared_ptr<Region::shared>> tmp;
					for (auto& i : channels) {
						//Region参数合法性检查
						if (!i.valid()) {
							return i.index;
						}
						tmp[i.index] = nullptr;
					}

					//Region.index重复性检查
					if (channels.size() != tmp.size()) {
						return PARAM_EXISTS;
					}
				}

				channels_.clear();
				numbers_.clear();

				for (auto& i : channels) {
					Region::shared pr = std::make_shared<Region>(logger_);
					pr->init(i);
					channels_.push_back(pr);
					numbers_[i.index] = pr;
				}

				//根据z轴次序进行排序
				std::sort(channels_.begin(), channels_.end(), [](Region::shared a, Region::shared b) {
					return a->getRegionCfg().zOrder < b->getRegionCfg().zOrder;
				});

				int ret = yuvMixer_->setRegions(channels);
				if (ret) {
					dbgi(logger_, "YUV mixer : regions joined failed! ret=[{}].", ret);
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				dbgt(logger_, "regions joined successfully!, regions=[{}]-[{}].", channels_[0]->getRegionCfg().index, channels_[channels_.size()-1]->getRegionCfg().index);

				return 0;
			}

			//增加添加一个Region
			// -1 : region参数错误
			// -2 : index重复
			// index : 成功
			virtual int addRegion(const RegionConfig& channel) override {
				if(!channel.valid()) {
					dbgi(logger_, "region param illegal!, region=[{}].", channel.index);
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				auto search = numbers_.find(channel.index);
				if (search != numbers_.end()) {
					dbgi(logger_, "region already exist!, region=[{}].", channel.index);
					return PARAM_EXISTS;
				}

				Region::shared pr = std::make_shared<Region>(logger_);
				pr->init(channel);
				channels_.push_back(pr);
				numbers_[channel.index] = pr;

				//根据z轴次序进行排序
				std::sort(channels_.begin(), channels_.end(), [](Region::shared a, Region::shared b) {
					return a->getRegionCfg().zOrder < b->getRegionCfg().zOrder;
				});

				dbgt(logger_, "region joined successfully!, region=[{}].", channel.index);

				return channel.index;
			}

			//当有数据时调用该方法输入数据
			virtual int input(int regionIndex, NVideoFrame* pkt) override {
				if (!isSupportCodecType(pkt->getCodecType())) {
					dbgi(logger_, "Unsupported codec type! index=[{}], Type=[{}].", regionIndex, NCodec::GetNameFor(pkt->getCodecType()));
					return NOT_SUPPORT_CODEC_TYPE;
				}

				auto region = numbers_.find(regionIndex);
				if (region == numbers_.end()) {
					dbgi(logger_, "Not found target region index! index=[{}].", regionIndex);
					return PARAM_NOT_EXISTS;
				}

				int ret = region->second->onInputFrame(pkt);
				if (ret) {
					return ret;
				}

				return yuvMixer_->inputRegionFrame(regionIndex, region->second->getDrawFrame());
			}

			//转码并异步输出视频流
			//转码的视频流参数由初始化转码器时传入的参数决定
			//调用transcode将编码的帧通过回调函数输出
			virtual int transcode(const AVFrame** frame) override {
				if (!outPacaket_) {
					return INTERNAL_PARAM_NOT_VAILD;
				}
				
				*frame = yuvMixer_->outputFrame();
				if (!*frame) {
					return 0;
				}

				//编码
				int ret = encode(*frame);
				if (ret) {
					if (ret < 0) {
						return ret;
					}
					return 0;
				}

				//输出
				if (onEncodeFrame_) {
					onEncodeFrame_(outPacaket_->data, outPacaket_->size);
				}

				av_packet_unref(outPacaket_);
				return 0;
			}

			//转码器是否开启
			virtual bool isOpened() const override{
				return nullptr != imgCodecCtx_;
			}

			//关闭转码器
			virtual void close() override {
				channels_.clear();
				numbers_.clear();

				if (outPacaket_) {
					av_packet_unref(outPacaket_);
				}

				if (imgCodecCtx_) {
					avcodec_close(imgCodecCtx_);
					imgCodecCtx_ = nullptr;
				}

				if (onEncodeFrame_) {
					onEncodeFrame_ = nullptr;
				}
			}

			//是否支持传入的编码类型
			virtual bool isSupportCodecType(const NCodec::Type typ) const override {
				return typ == NCodec::Type::H264
					|| typ == NCodec::Type::VP8;
			}
		private:
			//编码一帧视频帧
			int encode(const AVFrame* frame) {
				if (!outPacaket_) {
					dbge(logger_, "Error during encoding, NULL parameter!");
					return INTERNAL_PARAM_NOT_VAILD;
				}

				int ret = avcodec_send_frame(imgCodecCtx_, frame);
				if (ret) {
					if (AVERROR(EAGAIN) != ret) {
						dbge(logger_, "Error sending original frame to encoder!");
						return ERROR_ENCODE_VIDEO;
					}
					avcodec_receive_packet(imgCodecCtx_, outPacaket_);
					av_packet_unref(outPacaket_);
					avcodec_send_frame(imgCodecCtx_, frame);
				}

				ret = avcodec_receive_packet(imgCodecCtx_, outPacaket_);
				if (ret) {
					if (AVERROR(EAGAIN) != ret) {
						dbge(logger_, "Error receiving encoded frame from encoder!");
						return ERROR_ENCODE_VIDEO;
					}
					return 1;
				}

				return 0;
			}

			//初始化编解码器
			int initCodec() {

				if (!outPacaket_) {
					outPacaket_ = av_packet_alloc();
				}

				return initEncoder();
			}

			//初始化编码器
			int initEncoder() {

				if (!imgCodecCtx_) {
					int codecId = 0;
					AVDictionary* param = nullptr;

					codecId = convertFFCodecID(cfg_.outCodecType);
					if (codecId < 0) {
						dbge(logger_, "[encoder] not support codec type! NCodec::Type=[{}].", cfg_.outCodecType);
						return codecId;
					}

					AVCodec* pCodec = avcodec_find_encoder((AVCodecID)codecId);
					if (!pCodec) {
						dbge(logger_, "[encoder] Could not found video codec!  NCodec::Type=[{}].", cfg_.outCodecType);
						return NOT_SUPPORT_CODEC_TYPE;
					}

					imgCodecCtx_ = avcodec_alloc_context3(pCodec);
					if (!imgCodecCtx_) {
						dbge(logger_, "[encoder] Could not allocate video codec context!  NCodec::Type=[{}].", cfg_.outCodecType);
						return FAILED_INIT_ENCODER;
					}

					imgCodecCtx_->pix_fmt = (AVPixelFormat)OUT_FF_FMT;
					imgCodecCtx_->width = cfg_.width;
					imgCodecCtx_->height = cfg_.height;
					imgCodecCtx_->bit_rate = cfg_.bitrate;
					imgCodecCtx_->gop_size = 12;
					imgCodecCtx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

					if (NCodec::Type::H264 == cfg_.outCodecType) {
						paddingH264Codec(&param);
					}
					else if (NCodec::Type::VP8 == cfg_.outCodecType) {
						paddingVP8Codec(&param);
					}

					if (avcodec_open2(imgCodecCtx_, pCodec, &param) < 0) {
						dbge(logger_, "[encoder] Failed to open encoder!  NCodec::Type=[{}].", cfg_.outCodecType);
						return FAILED_INIT_ENCODER;
					}

					av_dict_free(&param);
					param = nullptr;
				}

				return 0;
			}

			//填充H264编码器
			void paddingH264Codec(AVDictionary** param) {
				imgCodecCtx_->time_base.num = 1;
				imgCodecCtx_->time_base.den = 25;
				//H264
				//imgCodecCtx_->me_range = 16;
				//imgCodecCtx_->max_qdiff = 4;
				//imgCodecCtx_->qcompress = 0.6;
				imgCodecCtx_->qmin = 10;
				imgCodecCtx_->qmax = 30;

				//Optional Param
				imgCodecCtx_->max_b_frames = 0;

				av_dict_set(param, "tune", "zerolatency", 0);      //zero delay
				av_dict_set(param, "profile", "baseline", 0);
			}

			//填充VP8编码器
			void paddingVP8Codec(AVDictionary** param) {
				imgCodecCtx_->time_base.num = 1;
				imgCodecCtx_->time_base.den = 90000;

				imgCodecCtx_->qmin = 4;
				imgCodecCtx_->qmax = 63;
			}
		};

		Transcoder::shared Transcoder::Create(const std::string& name) {
			return std::make_shared< TranscoderImpl>(name);
		}
	}	//video
}	//nmedia









