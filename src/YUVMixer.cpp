
#include <map>
#include <algorithm>
#include <set>

#include "NLogger.hpp"
#include "YUVMixer.hpp"
#include "NTErrorDefined.hpp"

#include "NMediaBasic.hpp"

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

namespace nmedia {
    namespace video{
        class YUVMixerImpl : public YUVMixer{
		public:
			static const int OUT_FF_FMT = AV_PIX_FMT_YUV420P;
        private:
            //区域中将被绘制的图像的参数
            struct ImgDrawParam {
                //The starting position on background
                int imgInBgx = -1;
                int imgInBgy = -1;
                //The starting point in the image to be printed
                int imgx = -1;
                int imgy = -1;
                NVideoSize              srcImgSize = { -1, -1 };
                NVideoSize              dstImgSize = { -1, -1 };
                NVideoSize              drawSize = { -1, -1 };
            };
            
            struct RegionImpl{
				using shared = std::shared_ptr< RegionImpl>;

                RegionConfig            region_;
                ImgDrawParam            imgConfig_;
                AVFrame*                swsFrame_ = nullptr;
                uint8_t*                swsBuf_ = nullptr;
                SwsContext*             imgConvertCtx_ = nullptr;
                
			public:
				~RegionImpl() {
					if (imgConvertCtx_) {
						sws_freeContext(imgConvertCtx_);
						imgConvertCtx_ = nullptr;
					}

					if (swsBuf_) {
						av_free(swsBuf_);
						swsBuf_ = nullptr;
					}

					if (swsFrame_) {
						av_frame_free(&swsFrame_);
					}
				}

				// 将输入帧调整至目标分辨率
				// -1 : 传入参数不合法
				// >= 0 : 转换图像的行高
				int zoom(const AVFrame* input) {
					if (!input
						|| !input->data) {
						return EXTERNAL_PARAM_NOT_VAILD;
					}

					if (!imgConvertCtx_) {
						return INTERNAL_PARAM_NOT_VAILD;
					}

					return sws_scale(imgConvertCtx_, (const uint8_t* const*)input->data, input->linesize, 0, input->height,
							swsFrame_->data, swsFrame_->linesize);
				}

				// 当传入的视频数据分辨率改变时，需要调用改方法
				// 错误返回负值，否则返回0
				// -1 : 传入参数不合法
				// -2 : 创建缩放器失败
				// -3 : 填充目标区域失败
				int onChangeResolution(const int srcWidth, const int srcHeight, const AVPixelFormat typ) {
					if (srcWidth <= 0
						|| srcHeight <= 0
						|| typ < 0) {
						return EXTERNAL_PARAM_NOT_VAILD;
					}

					int ret = 0;

					getDstResolution(srcWidth, srcHeight);

					calcImgRelatePos(imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height);

					calcImgDrawOnBgSize();

					if (imgConvertCtx_) {
						sws_freeContext(imgConvertCtx_);
					}

					imgConvertCtx_ = sws_getContext(srcWidth, srcHeight, typ
						, imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height
						, (AVPixelFormat)OUT_FF_FMT, SWS_BICUBIC, NULL, NULL, NULL);
					if (!imgConvertCtx_) {
						//dbge(logger_, "Could not init resolution converter! index=[{}].", bgConfig_.index);
						return FAILED_INIT_CONVERTER;
					}
					
					if (!swsFrame_) {
						swsFrame_ = av_frame_alloc();
					}

					if(swsBuf_) {
						av_free(swsBuf_);
					}
					
					swsBuf_ = (uint8_t*)av_malloc(
						av_image_get_buffer_size((AVPixelFormat)OUT_FF_FMT, imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height, 1)
					);

					if (av_image_fill_arrays(swsFrame_->data, swsFrame_->linesize
						, swsBuf_, (AVPixelFormat)OUT_FF_FMT
						, imgConfig_.dstImgSize.width
						, imgConfig_.dstImgSize.height, 1) < 0) {
						//dbge(logger_, "Could not init swsFrame buffer! index=[{}].", bgConfig_.index);
						return FAILED_FILL_BUFFER;
					}

					swsFrame_->width = imgConfig_.dstImgSize.width;
					swsFrame_->height = imgConfig_.dstImgSize.height;
					swsFrame_->format = OUT_FF_FMT;

					return 0;
				}

			private:
				//获取当前适应模式下应缩放成的分辨率
				inline void getDstResolution(int srcWidth, int srcHeight) {

					if (ScalingMode::None == region_.scalinglMode) {
						imgConfig_.dstImgSize.width = srcWidth;
						imgConfig_.dstImgSize.height = srcHeight;
					}
					else if (ScalingMode::AspectFit == region_.scalinglMode) {      //TODO:这个模式编写的有没有问题

					 //以一条边为基准缩放
						imgConfig_.dstImgSize.width = region_.width;
						imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
						if (imgConfig_.dstImgSize.height > region_.height) {
							//以另一条边为基准缩放
							imgConfig_.dstImgSize.height = region_.height;
							imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
						}
					}
					else if (ScalingMode::AspectFill == region_.scalinglMode) {

						//以一条边为基准缩放
						imgConfig_.dstImgSize.width = region_.width;
						imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
						if (imgConfig_.dstImgSize.height < region_.height) {
							//以另一条边为基准缩放
							imgConfig_.dstImgSize.height = region_.height;
							imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
						}

					}
					else if (ScalingMode::Fill == region_.scalinglMode) {
						imgConfig_.dstImgSize.width = region_.width;
						imgConfig_.dstImgSize.height = region_.height;
					}

					//按比例求得对齐比例分辨率，ffmpeg规定缩放的目标尺寸必须为16的倍数
					float ratio = 1.0f * imgConfig_.dstImgSize.width / imgConfig_.dstImgSize.height;
					imgConfig_.dstImgSize.height = (imgConfig_.dstImgSize.height >> 4) << 4;
					imgConfig_.dstImgSize.width = (int(ratio * imgConfig_.dstImgSize.height) >> 4) << 4;
				}

				//Get the position of the image in the background and get the starting point in the image to be printed
				//计算出图像在区域中的起始点以及图像中被绘制像素的起始点
				inline void calcImgRelatePos(const int imgWidth, const int imgHeight) {
					imgConfig_.imgInBgx = region_.x;
					imgConfig_.imgInBgy = region_.y;
					imgConfig_.imgx = 0;
					imgConfig_.imgy = 0;

					if (ScalingMode::None == region_.scalinglMode) {
						//图小背景大，应该画全图
						//图大背景小，应该画满背景
						if (region_.width > imgWidth) {
							// imgConfig_.startPosInImg.first = 0;
							imgConfig_.imgInBgx += bisectSub(region_.width, imgWidth);
						}
						else {
							imgConfig_.imgx = bisectSub(imgWidth, region_.width);
							imgConfig_.imgInBgx += 0;
						}

						if (region_.height > imgHeight) {
							// imgConfig_.imgy = 0;
							imgConfig_.imgInBgy += bisectSub(region_.height, imgHeight);
						}
						else {
							imgConfig_.imgy = bisectSub(imgHeight, region_.height);
							imgConfig_.imgInBgy += 0;
						}
					}
					else if (ScalingMode::AspectFit == region_.scalinglMode) {
						imgConfig_.imgInBgx += abs(bisectSub(imgWidth, region_.width));
						imgConfig_.imgInBgy += abs(bisectSub(imgHeight, region_.height));
					}
					else if (ScalingMode::AspectFill == region_.scalinglMode) {
						if (region_.width < imgWidth) {
							imgConfig_.imgx = bisectSub(imgWidth, region_.width);
							// imgConfig_.imgInBgx += 0;
						}
						else if (region_.height < imgHeight) {
							imgConfig_.imgy = bisectSub(imgHeight, region_.height);
							// imgConfig_.imgInBgy += 0;
						}
					}
					else if (ScalingMode::Fill == region_.scalinglMode) {
						// imgConfig_.imgInBgx += 0;
						// imgConfig_.imgInBgy += 0;
					}
				}

				inline void calcImgDrawOnBgSize() {
					// imgConfig_.drawSize;
					// imgConfig_.imgInBgx;		//A1.x
					// imgConfig_.imgInBgy;		//A1.y
					// imgConfig_.imgSize;		//x2,y2
					// region_;					//A0  x1,y1

					if (imgConfig_.imgInBgx - region_.x + imgConfig_.dstImgSize.width
						> region_.width) {
						imgConfig_.drawSize.width = region_.width - imgConfig_.imgInBgx + region_.x;
					}
					else {
						imgConfig_.drawSize.width = imgConfig_.dstImgSize.width;
					}

					if (imgConfig_.imgInBgy - region_.y + imgConfig_.dstImgSize.height
						> region_.height) {
						imgConfig_.drawSize.height = region_.height - imgConfig_.imgInBgy + region_.y;
					}
					else {
						imgConfig_.drawSize.height = imgConfig_.dstImgSize.height;
					}

				}

				inline int bisectSub(int val1, int val2) {
					return (val1 - val2) / 2;
				}

				inline int abs(int val) {
					return val > 0 ? val : (-val);
				}
            };
		private:
			NLogger::shared logger_ = nullptr;
			std::vector< RegionImpl::shared> regions_;
			std::map<int, RegionImpl::shared> numbers_;
			uint32_t					backgroundColor_ = 0x008080;		//YUV	黑
			AVFrame*					outFrame_ = nullptr;
			uint8_t*					outBuf_ = nullptr;
        public:

			YUVMixerImpl(const std::string& name) :logger_(NLogger::Get(name)) { }

            ~YUVMixerImpl(){}
            
            // 设置输出图像尺寸
			// bkground_color : RGB24
            virtual int outputConfig(int width, int height, uint32_t bkground_color) override{
				if (1 > width
				||  1 > height) {
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				av_log_set_level(AV_LOG_QUIET);

				if (!outFrame_) {
					outFrame_ = av_frame_alloc();
				}

				outFrame_->width = width;
				outFrame_->height = height;
				outFrame_->format = OUT_FF_FMT;

				{
					uint8_t R = bkground_color >> 16;
					uint8_t	G = bkground_color >> 8;
					uint8_t B = bkground_color;
					uint8_t Y = 0.299 * R + 0.587 * G + 0.114 * B;
					uint8_t V = 0.500 * R - 0.419 * G - 0.081 * B + 128;
					uint8_t U = -0.169 * R - 0.331 * G + 0.500 * B + 128;

					backgroundColor_ = ((uint32_t)Y << 16) | ((uint32_t)U << 8) | ((uint32_t)V);
				}

				if (outBuf_) {
					av_free(outBuf_);
				}

				outBuf_ = (uint8_t*)av_malloc(
					av_image_get_buffer_size((AVPixelFormat)OUT_FF_FMT, outFrame_->width, outFrame_->height, 1)
				);

				if (av_image_fill_arrays(outFrame_->data, outFrame_->linesize
					, outBuf_, (AVPixelFormat)OUT_FF_FMT
					, outFrame_->width
					, outFrame_->height, 1) < 0) {
					//dbge(logger_, "Could not init swsFrame buffer! index=[{}].", bgConfig_.index);
					return FAILED_FILL_BUFFER;
				}

				//重置目标yuv图像为某种单一颜色
				return resetBgByYuv420p(backgroundColor_);
            }
            
            // 添加一个Region
            // 返回值 >=0 表示成功，返回值为Region的index
			// -1 ： region参数不合法
			// -2 ： region.index重复
            virtual int addRegion(const RegionConfig& r) override{
				if (!r.valid()) {
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				auto search = numbers_.find(r.index);
				if (search != numbers_.end()) {
					return PARAM_EXISTS;
				}

				RegionImpl::shared regionImp = std::make_shared< RegionImpl>();
				regionImp->region_ = r;

				regions_.push_back(regionImp);
				numbers_[r.index] = regionImp;

				//根据z轴次序进行排序
				std::sort(regions_.begin(), regions_.end(), [](RegionImpl::shared a, RegionImpl::shared b) {
					return a->region_.zOrder < b->region_.zOrder;
				});

                return r.index;
            }
            
            // 设置Region列表，旧的Region列表被清空并设置成新的Region列表
			// 如果新的region列表中有region参数不正确，则不会有任何region被成功设置
			// 错误返回错误码（负值），region参数错误返回index，正确返回0
			// -1 ： index重复
			// < 0 : region参数非法，返回非法index
            virtual int setRegions(const std::vector<RegionConfig>& v) override{
				{
					std::map<int, std::shared_ptr<RegionImpl::shared>> tmp;
					for (auto& i : v) {
						//Region参数合法性检查
						if (!i.valid()) {
							return i.index;
						}
						tmp[i.index] = nullptr;
					}

					//Region.index重复性检查
					if (v.size() != tmp.size()) {
						return PARAM_EXISTS;
					}
				}

				regions_.clear();
				numbers_.clear();

				for (auto& i : v) {
					RegionImpl::shared regionImp = std::make_shared< RegionImpl>();
					regionImp->region_ = i;

					regions_.push_back(regionImp);
					numbers_[i.index] = regionImp;
				}

				//根据z轴次序进行排序
				std::sort(regions_.begin(), regions_.end(), [](RegionImpl::shared a, RegionImpl::shared b) {
					return a->region_.zOrder < b->region_.zOrder;
				});

                return 0;
            }
            
            // 输入1帧数据到指定Region，错误返回负值，否则返回0
			// -1 : frame为null
			// -2 : 没有找到对应region
			// -3 : 分辨率变更出错
            virtual int inputRegionFrame(int region_index,
                                         const AVFrame * frame) override{
				if (!frame
					|| !frame->data) {
					return EXTERNAL_PARAM_NOT_VAILD;
				}

				auto search = numbers_.find(region_index);
				if (search == numbers_.end()) {
					return PARAM_NOT_EXISTS;
				}

				if (frame->width != search->second->imgConfig_.srcImgSize.width
				||	frame->height != search->second->imgConfig_.srcImgSize.height) {
					int ret = search->second->onChangeResolution(frame->width, frame->height, (AVPixelFormat)frame->format);
					if (ret < 0) {
						dbge(logger_, "change resolution error! index=[{}], error=[{}].", search->first, ret);
						return FAILED_INIT_CONVERTER;
					}
					search->second->imgConfig_.srcImgSize.width = frame->width;
					search->second->imgConfig_.srcImgSize.height = frame->height;
				}

                return search->second->zoom(frame);
            }
            
            // 输出1帧图像
            virtual const AVFrame * outputFrame() override{
				if (resetBgByYuv420p( backgroundColor_) < 0) {
					return nullptr;
				}

				for (auto i : regions_) {
					if (brushYUV420P(i->imgConfig_, i->swsFrame_) < 0) {
						return nullptr;
					}
				}

                return outFrame_;
            }
            
		private:
			int resetBgByYuv420p(uint32_t yuvColor) {
				if (!outFrame_
					|| !outFrame_->data) {
					return INTERNAL_PARAM_NOT_VAILD;
				}

				memset(outFrame_->data[0], (uint8_t)(yuvColor>>16), outFrame_->width * outFrame_->height * 1 * sizeof(uint8_t));
				memset(outFrame_->data[1], (uint8_t)(yuvColor>>8), outFrame_->width * outFrame_->height / 4 * sizeof(uint8_t));
				memset(outFrame_->data[2], (uint8_t)(yuvColor), outFrame_->width * outFrame_->height / 4 * sizeof(uint8_t));

				return 0;
			}

			//将多路区域画入画布中
			int brushYUV420P(const ImgDrawParam& imgConfig, const AVFrame* src) {

				if (!outBuf_ 
				|| !outFrame_) {
					dbge(logger_, "An error occurred while painting, the parameter is NULL!");
					return INTERNAL_PARAM_NOT_VAILD;
				} else if (!src 
					|| !src->data) {
					return 0;
				}

				int drawWidth = imgConfig.drawSize.width;
				int drawHeight = imgConfig.drawSize.height;

				if (imgConfig.drawSize.width + imgConfig.imgInBgx > outFrame_->width) {
					drawWidth = outFrame_->width - imgConfig.imgInBgx;
				}

				if (imgConfig.drawSize.height + imgConfig.imgInBgy > outFrame_->height) {
					drawHeight = outFrame_->height - imgConfig.imgInBgy;
				}
				//int drawWidth = (imgConfig.drawSize.width + imgConfig.imgInBgx > outFrame_->width) ? (outFrame_->width - imgConfig.imgInBgx) ? imgConfig.drawSize.width;

				//易看版本
				//int nYIndex = 0;
				//int nUVIndex = 0;

				//for (int i = 0; i < imgHeight; ++i) {
				//	//Y
				//	memcpy(dst->data[0] + (i + y) * dst->width + x, src->data[0] + (nYIndex + imgy) * src->width + imgx, imgWidth);
				//	++nYIndex;
				//}

				//for (int i = 0; i < imgHeight / 2; ++i) {
				//	//U
				//	memcpy(dst->data[1] + (i + y / 2) * (dst->width / 2) + x / 2, src->data[1] + (nUVIndex + imgy / 2) * (src->width / 2) + imgx / 2 , imgWidth / 2);
				//	//V
				//	memcpy(dst->data[2] + (i + y / 2) * (dst->width / 2) + x / 2, src->data[2] + (nUVIndex + imgy / 2) * (src->width / 2) + imgx / 2, imgWidth / 2);
				//	++nUVIndex;
				//}

				//简化版本
				for (int i = 0; i < drawHeight; ++i) {
					//Y
					memcpy(outFrame_->data[0] + (i + imgConfig.imgInBgy) * outFrame_->width + imgConfig.imgInBgx
						, src->data[0] + (i + imgConfig.imgy) * src->width + imgConfig.imgx
						, drawWidth);

					if (!(i % 2)) {
						//U
						memcpy(outFrame_->data[1] + (i + imgConfig.imgInBgy) / 2 * (outFrame_->width / 2) + imgConfig.imgInBgx / 2
							, src->data[1] + (i + imgConfig.imgy) / 2 * (src->width / 2) + imgConfig.imgx / 2
							, drawWidth / 2);

						//V
						memcpy(outFrame_->data[2] + (i + imgConfig.imgInBgy) / 2 * (outFrame_->width / 2) + imgConfig.imgInBgx / 2
							, src->data[2] + (i + imgConfig.imgy) / 2 * (src->width / 2) + imgConfig.imgx / 2
							, drawWidth / 2);
					}
				}

				return 0;
			}
        };
        
        YUVMixer::shared YUVMixer::Create(const std::string& name){
            return std::make_shared<YUVMixerImpl>(name);
        }
    }
}
