#ifndef NVideoTranscoder_hpp
#define NVideoTranscoder_hpp

#include  <memory>
#include <utility>
#include <algorithm>
#include <map>

#include "NMediaFrame.hpp"
#include "NLogger.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/log.h"
};



namespace nmedia {
    namespace video{

        static inline int convertFFCodecID(NCodec::Type typ) {
            if(NCodec::Type::VP8 == typ) {
                return AV_CODEC_ID_VP8;
            } else if (NCodec::Type::H264 == typ) {
				return AV_CODEC_ID_H264;
            }
            return -1;
        }

        static const int OUT_FF_FMT = AV_PIX_FMT_YUV420P;

        class Region{
		public:
			//contentMode : https://github.com/ksvc/KSYMediaPlayer_iOS/wiki/contentMode
			//��Ӧģʽ
			enum contentMode {
				UNKOWN,
				ScalingModeNone,        //������
				ScalingModeAspectFit,   //ͬ�����䣬�ȱ����ţ�ĳ��������кڱ�
				ScalingModeAspectFill,  //ͬ����䣬�޺ڱߣ�ĳ���������ʾ���ݿ��ܱ��ü�
				ScalingModeFill         //������䣬�ƻ�ԭ��Ƶ��������ԭʼ��Ƶ������һ��
			};

			//������Ĳ���
			struct RegionConfig {
				//       ���궨��
				//          x
				//  ------------------->
				//  | ----------------
				//  | |              |
				// y| |              |
				//  | |              |
				//  | ----------------
				//  v
				int             index = -1;
				int             x = -1;
				int             y = -1;
				int             width = -1;
				int             height = -1;
				int             zOrder = -1;        //zOrder����Խ�����ڲ��Խ�ߣ�Խ���ᱻ�ڵ���
				int             fillMode = contentMode::ScalingModeAspectFit;

				bool isRegionLegality() const {
					return (0 < index)
						&& (-1 < x)
						&& (-1 < y)
						&& (0 < width)
						&& (0 < height)
						&& (0 < zOrder)
						&& (contentMode::UNKOWN < fillMode);
				}

				std::string dump() const {
					return fmt::format("index={},x={},y={}\nwidth={},height={},zOrder={}\nfillMode={}\n"
						, index, x, y, width, height, zOrder, fillMode);
				}
			};

			//�����н������Ƶ�ͼ��Ĳ���
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
        private:
            NLogger::shared         logger_;
            RegionConfig            bgConfig_;
            //Image position in the background
            ImgDrawParam            imgConfig_;
            AVPacket*               imgPacket_ = nullptr;
            AVFrame*                inFrame_ = nullptr;
            AVFrame*                swsFrame_ = nullptr;
            uint8_t*                swsBuf_ = nullptr;
            SwsContext*             imgConvertCtx_ = nullptr;
            AVCodecParserContext*   imgParserCtx_ = nullptr;
            AVCodecContext*         imgCodecCtx_ = nullptr;
            bool                    bReinitCvt_ = false;

        public:
            using shared = std::shared_ptr<Region>;

			//FILE* fp = nullptr;
            Region(NLogger::shared logger) :logger_(logger) {
				//fp = fopen("C:/Users/Li/Desktop/output_640x480.h264p", "wb");
			}

            Region(const std::string& name) :logger_(NLogger::Get(name)) { }

            virtual ~Region() {
                close();
				//if (fp) {
				//	fclose(fp);
				//	fp = nullptr;
				//}
            }

			//��ʼ��������
            int init(const RegionConfig& config) {
                if(!config.isRegionLegality()) {
                    return -1;
                }

                bgConfig_ = config;
                return 0;
            }

			//���������Ƶ���ݷֱ��ʸı�ʱ����Ҫ���øķ���
            int onChangeResolution(const int srcWidth, const int srcHeight, const NCodec::Type typ) {
                if(srcWidth <= 0 
                || srcHeight <= 0
                || typ < 0) {
                    return -1;
                }

                int ret = 0;

                bReinitCvt_ = true;

                getDstResolution(srcWidth, srcHeight);
                
                calcImgRelatePos(imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height);

                calcImgDrawOnBgSize();

                if(!imgCodecCtx_) {
                    ret = initDecoder(typ);
                    if(ret < 0) {
                        return ret;
                    }
                }

                if(!imgParserCtx_) {
                    ret = initImgParser(typ);
                    if(ret < 0) {
                        return ret;
                    }
                }

                return 0;
            }

			//������������ʱ��������Ӧ�õ����������
			//���ﴫ�������Ӧ��Я���ֱ�����Ϣ
            int onInputFrame(NVideoFrame* inFrame) {

                NCodec::Type typ = inFrame->getCodecType();
                NVideoSize size;

                if(NCodec::H264 == typ) {
                    size = inFrame->videoSize();
                } else if (NCodec::VP8 == typ) {
                    size = inFrame->videoSize();
                }

                if(size != imgConfig_.srcImgSize) {
                    int ret = onChangeResolution(size.width, size.height, typ);
                    if(ret < 0) {
                        dbge(logger_, "change resolution error! index=[{}], error=[{}].", bgConfig_.index, ret);
                        return -1;
                    }
					imgConfig_.srcImgSize = size;
                }

                return transcoder(inFrame, size);
            }

			//��ȡ�����н������Ƶ���������
            const AVFrame* getDrawFrame() const {
                return swsFrame_;
            }
			
			//��ȡ����������������Ϣ
            const ImgDrawParam& getDrawParam() const {
                return imgConfig_;
            }

			//��ȡ�����������Ϣ
            const RegionConfig& getRegionCfg() const {
                return bgConfig_;
            }

            bool isOpened() const {
                return imgCodecCtx_ && imgParserCtx_;
            }

			//ˢ������Ľ�����
            void flushDecoder() const {
                if(!isOpened()) {
                    return ;
                }

                imgPacket_->data = nullptr;
                imgPacket_->size = 0;

                int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
                if (ret) {
                    if (AVERROR(EAGAIN) == ret) {
                        avcodec_receive_frame(imgCodecCtx_, inFrame_);
                        av_frame_unref(inFrame_);
                        avcodec_send_packet(imgCodecCtx_, imgPacket_);
                    } else {
                        dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", bgConfig_.index, ret);
                        return ;
                    }
                }

                ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
                if (ret) {
                    if (AVERROR(EAGAIN) == ret) {
						return;
                    }
					char arr[1024] = { 0 };
					av_strerror(ret, arr, 1024);
                    dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", bgConfig_.index, arr);
                        ret = -3;
						return;
                }

                if(imgConvertCtx_) {
                    sws_scale(imgConvertCtx_, (const uint8_t* const*)inFrame_->data, inFrame_->linesize, 0, imgCodecCtx_->height,
                        swsFrame_->data, swsFrame_->linesize);
                }

                av_frame_unref(inFrame_);
            }

			//�ر�����
            void close() {
                flushDecoder();

                if(imgConvertCtx_) {
                    sws_freeContext(imgConvertCtx_);
                    imgConvertCtx_ = nullptr;
                }

                if(swsFrame_) {
                    av_frame_free(&swsFrame_);
                }

                if(swsBuf_) {
                    av_free(swsBuf_);
                    swsBuf_ = nullptr;
                }

                if(imgPacket_) {
                    av_packet_unref(imgPacket_);
                    imgPacket_ = nullptr;
                }

                if(inFrame_) {
                    av_frame_free(&inFrame_);
                }

                if(imgParserCtx_) {
                    av_parser_close(imgParserCtx_);
                    imgParserCtx_ = nullptr;
                }

                if(imgCodecCtx_) {
                    avcodec_close(imgCodecCtx_);
                    imgCodecCtx_ = nullptr;
                }
            }

			const AVFrame* getSrcFrame() {
				return inFrame_;
			}

        private:
			//����ת�룬�������Ƶ֡���Բ�Я���ֱ�����Ϣ����Ϊ��̽�����Զ���̽��Ƶ֡�ֱ���
			//size��ʾ��Ƶ֡Ӧ�ñ����ŵĴ�С������ˢ������ģ��
			//���ｫ����ģ����������Ҫ����Ϊ��̽�����Ի�ȡ��Ƶ֡��֡��ʽ������ģ��ĳ�ʼ����Ҫ���֡��ʽ��
			inline int transcoder(NVideoFrame* frame, const NVideoSize& size) {
				if (!isOpened()) {
					dbge(logger_, "transcoder is not open! index=[{}].", bgConfig_.index);
					return -1;
				}

				imgPacket_->data = frame->data();
				imgPacket_->size = frame->size();

				int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						avcodec_receive_frame(imgCodecCtx_, inFrame_);
						av_frame_unref(inFrame_);
						avcodec_send_packet(imgCodecCtx_, imgPacket_);
					}
					else {
						dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", bgConfig_.index, ret);
						ret = -2;
						return -2;
					}
				}

				ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
				if (ret) {
					if (AVERROR(EAGAIN) == ret) {
						return 0;
					}
					dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", bgConfig_.index, ret);
					ret = -3;
					return -3;
				}

				if (bReinitCvt_) {
					ret = reinitRConverter(size.width, size.height);
					if (ret < 0) {
						ret = -4;
						return -4;
					}
				}

				sws_scale(imgConvertCtx_, (const uint8_t* const*)inFrame_->data, inFrame_->linesize, 0, imgCodecCtx_->height,
					swsFrame_->data, swsFrame_->linesize);

				av_packet_unref(imgPacket_);
				return 0;
			}

			inline int transcoder1(NVideoFrame* frame, const NVideoSize& size) {
				if (!isOpened()) {
					dbge(logger_, "transcoder is not open! index=[{}].", bgConfig_.index);
					return -1;
				}

				int frameSize = frame->size();
				const uint8_t* pBuf = frame->data();
				int ret = 0;

				while (frameSize) {

					int parse_len = av_parser_parse2(imgParserCtx_, imgCodecCtx_
						, &imgPacket_->data, &imgPacket_->size
						, pBuf, frameSize
						, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);

					pBuf += parse_len;
					frameSize -= parse_len;

					if (imgPacket_->size) {
						//dbgi(logger_, "imgPacket_.size=[{}].", imgPacket_->size);
						//fwrite(&imgPacket_->size, sizeof(uint32_t), 1, fp);

						//fwrite(imgPacket_->data, sizeof(uint8_t), imgPacket_->size, fp);

						int ret = avcodec_send_packet(imgCodecCtx_, imgPacket_);
						if (ret) {
							if (AVERROR(EAGAIN) == ret) {
								avcodec_receive_frame(imgCodecCtx_, inFrame_);
								av_frame_unref(inFrame_);
								avcodec_send_packet(imgCodecCtx_, imgPacket_);
							}
							else {
								dbge(logger_, "send frame to decoder error. index=[{}], error=[{}].", bgConfig_.index, ret);
								ret = -2;
								return -2;
							}
						}

						ret = avcodec_receive_frame(imgCodecCtx_, inFrame_);
						if (ret) {
							if (AVERROR(EAGAIN) == ret) {
								return 0;
							}
							dbge(logger_, "receive frame from decoder error. index=[{}], error=[{}].", bgConfig_.index, ret);
							ret = -3;
							return -3;
						}

						if (bReinitCvt_) {
							ret = reinitRConverter(size.width, size.height);
							if (ret < 0) {
								ret = -4;
								return -4;
							}
						}

						sws_scale(imgConvertCtx_, (const uint8_t* const*)inFrame_->data, inFrame_->linesize, 0, imgCodecCtx_->height,
							swsFrame_->data, swsFrame_->linesize);

						av_packet_unref(imgPacket_);
					}
				}

				//av_frame_unref(inFrame_);
				av_packet_unref(imgPacket_);
				return 0;
			}

			//��ʼ��������
            inline int initDecoder(const NCodec::Type typ) {
                if(!imgCodecCtx_) {
                    int codecId = convertFFCodecID(typ);
                    if(codecId <= 0) {
                        dbge(logger_, "Unsupported decoder type!  index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -1;
                    }

                    AVCodec* pCodec = avcodec_find_decoder((AVCodecID)codecId);
                    if (!pCodec) {
                        dbge(logger_, "Can not find decoder! index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -2;
                    }

                    imgCodecCtx_ = avcodec_alloc_context3(pCodec);
                    if (!imgCodecCtx_) {
                        dbge(logger_, "Could not allocate video codec context! index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -3;
                    }

                    if (avcodec_open2(imgCodecCtx_, pCodec, NULL) < 0) {
                        dbge(logger_, "Could not open codec! index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -4;
                    }
                }

                if(!inFrame_) {
                    inFrame_ = av_frame_alloc();
                }

                return 0;
            }

			//���³�ʼ��ͼ������ģ�飬����������Ƶ���ֱ����б仯ʱʹ��
            inline int reinitRConverter(const int srcWidth, const int srcHeight) {
                if(0 >= imgConfig_.dstImgSize.width
                || 0 >= imgConfig_.dstImgSize.height) {
                    return -1;
                }

                if(imgConvertCtx_) {
                    sws_freeContext(imgConvertCtx_);
                    imgConvertCtx_ = nullptr;
                }

                imgConvertCtx_ = sws_getContext(srcWidth, srcHeight, imgCodecCtx_->pix_fmt
                                                ,imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height
                                                , (AVPixelFormat)OUT_FF_FMT, SWS_BICUBIC, NULL, NULL, NULL);
                if(!imgConvertCtx_) {
                    dbge(logger_, "Could not init resolution converter! index=[{}].", bgConfig_.index);
                    return -2;
                }

                if(!swsFrame_) {
                    swsFrame_ = av_frame_alloc();       //���ڽ����Լ�ʹ�ú����av_frame_unref(pFrame);��������ڴ�й©,������������Ӱ��
                }

                if(swsBuf_) {
                    av_free(swsBuf_);
                    swsBuf_ = nullptr;
                }

                swsBuf_ = (uint8_t*)av_malloc(
                                    av_image_get_buffer_size((AVPixelFormat)OUT_FF_FMT, imgConfig_.dstImgSize.width, imgConfig_.dstImgSize.height, 1)
                                    );

                if(av_image_fill_arrays(swsFrame_->data, swsFrame_->linesize
                                    , swsBuf_, (AVPixelFormat)OUT_FF_FMT
                                    , imgConfig_.dstImgSize.width
                                    , imgConfig_.dstImgSize.height, 1) < 0) {
                    dbge(logger_, "Could not init swsFrame buffer! index=[{}].", bgConfig_.index);
                    return -3;
                }

                memset(swsFrame_->data[0], 0x00, imgConfig_.dstImgSize.width * imgConfig_.dstImgSize.height * 1 * sizeof(uint8_t));
                memset(swsFrame_->data[1], 0x80, imgConfig_.dstImgSize.width * imgConfig_.dstImgSize.height / 4 * sizeof(uint8_t));
                memset(swsFrame_->data[2], 0x80, imgConfig_.dstImgSize.width * imgConfig_.dstImgSize.height / 4 * sizeof(uint8_t));

                swsFrame_->width = imgConfig_.dstImgSize.width;
                swsFrame_->height = imgConfig_.dstImgSize.height;
                swsFrame_->format = OUT_FF_FMT;

                bReinitCvt_ = false;
                return 0;
            }

			//��ʼ��ͼ����̽�������ڴ���������ʶ��һ��֡������Ƶ֡
            inline int initImgParser(const NCodec::Type typ) {
                if(!imgParserCtx_) {
                    int codecId = convertFFCodecID(typ);
                    if(codecId <= 0) {
                        dbge(logger_, "Unsupported decoder type!  index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -1;
                    }

                    imgParserCtx_ = av_parser_init(codecId);
                    //avParserContext->flags |= PARSER_FLAG_ONCE;
                    if (!imgParserCtx_) {
                        dbge(logger_, "Could not init avParserContext! index=[{}], NCodec::Type=[{}].", bgConfig_.index, typ);
                        return -2;
                    }
                }

                if(!imgPacket_) {
                    imgPacket_ = av_packet_alloc();
                }

                return 0;
            }

			//��ȡ��ǰ��Ӧģʽ��Ӧ���ųɵķֱ���
            inline void getDstResolution(int srcWidth, int srcHeight) {

                if(contentMode::ScalingModeNone == bgConfig_.fillMode) {
                    imgConfig_.dstImgSize.width = srcWidth;
                    imgConfig_.dstImgSize.height = srcHeight;
                } else if(contentMode::ScalingModeAspectFit == bgConfig_.fillMode) {      //TODO:���ģʽ��д����û������
                    
                    //��һ����Ϊ��׼����
                    imgConfig_.dstImgSize.width = bgConfig_.width;
                    imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
                    if(imgConfig_.dstImgSize.height > bgConfig_.height) {
                        //����һ����Ϊ��׼����
                        imgConfig_.dstImgSize.height = bgConfig_.height;
                        imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
                    }

                } else if(contentMode::ScalingModeAspectFill == bgConfig_.fillMode) {

                    //��һ����Ϊ��׼����
                    imgConfig_.dstImgSize.width = bgConfig_.width;
                    imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
                    if(imgConfig_.dstImgSize.height < bgConfig_.height) {
                        //����һ����Ϊ��׼����
                        imgConfig_.dstImgSize.height = bgConfig_.height;
                        imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
                    }

                } else if(contentMode::ScalingModeFill == bgConfig_.fillMode) {
                    imgConfig_.dstImgSize.width = bgConfig_.width;
                    imgConfig_.dstImgSize.height = bgConfig_.height;
                } 
            }

            //Get the position of the image in the background and get the starting point in the image to be printed
			//�����ͼ���������е���ʼ���Լ�ͼ���б��������ص���ʼ��
            inline void calcImgRelatePos(const int imgWidth, const int imgHeight) {
                imgConfig_.imgInBgx = bgConfig_.x;
                imgConfig_.imgInBgy = bgConfig_.y;
                imgConfig_.imgx = 0;
                imgConfig_.imgy = 0;

                if(contentMode::ScalingModeNone == bgConfig_.fillMode) {
                    //ͼС������Ӧ�û�ȫͼ
                    //ͼ�󱳾�С��Ӧ�û�������
                    if(bgConfig_.width > imgWidth) {
                        // imgConfig_.startPosInImg.first = 0;
                        imgConfig_.imgInBgx += bisectSub(bgConfig_.width, imgWidth);
                    } else {
                        imgConfig_.imgx = bisectSub(imgWidth, bgConfig_.width);
                        imgConfig_.imgInBgx += 0;
                    }

                    if (bgConfig_.height > imgHeight) {
                        // imgConfig_.imgy = 0;
                        imgConfig_.imgInBgy += bisectSub(bgConfig_.height, imgHeight);
                    } else {
                        imgConfig_.imgy = bisectSub(imgHeight, bgConfig_.height);
                        imgConfig_.imgInBgy += 0;
                    }
                } else if(contentMode::ScalingModeAspectFit == bgConfig_.fillMode) {
                    imgConfig_.imgInBgx += abs(bisectSub(imgWidth, bgConfig_.width));
                    imgConfig_.imgInBgy += abs(bisectSub(imgHeight, bgConfig_.height));
                } else if(contentMode::ScalingModeAspectFill == bgConfig_.fillMode) {
                    if(bgConfig_.width < imgWidth) {
                        imgConfig_.imgx = bisectSub(imgWidth, bgConfig_.width);
                        // imgConfig_.imgInBgx += 0;
                    } else if (bgConfig_.height < imgHeight) {
                        imgConfig_.imgx = bisectSub(imgHeight, bgConfig_.height);
                        // imgConfig_.imgInBgy += 0;
                    }
                } else if(contentMode::ScalingModeFill == bgConfig_.fillMode) {
                    // imgConfig_.imgInBgx += 0;
                    // imgConfig_.imgInBgy += 0;
                }
            }

            inline void calcImgDrawOnBgSize() {
                // imgConfig_.drawSize;
                // imgConfig_.imgInBgx;  //A1.x
                // imgConfig_.imgInBgy;  //A1.y
                // imgConfig_.imgSize;     //x2,y2
                // bgConfig_;              //A0  x1,y1

                if(imgConfig_.imgInBgx - bgConfig_.x + imgConfig_.dstImgSize.width 
                                                                    > bgConfig_.width) {
                    imgConfig_.drawSize.width = bgConfig_.width - imgConfig_.imgInBgx + bgConfig_.x;
                } else {
                    imgConfig_.drawSize.width = imgConfig_.dstImgSize.width;
                }

                if(imgConfig_.imgInBgy - bgConfig_.y + imgConfig_.dstImgSize.height 
                                                                    > bgConfig_.height) {
                    imgConfig_.drawSize.height = bgConfig_.height - imgConfig_.imgInBgy + bgConfig_.y;
                } else {
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
        
        //������ʽΪYUV420P
        //ռ���ڴ���㣺width * height * (3 / 2)

        //1. �в�λ�������Ҫ���ǲ�����⣬�������²㱻�ϲ㲻��ȫ�ڵ��Ļ�������ͼ��ʱ��Ӧ����ô����
        //Ŀǰ�İ취����������ͨ���ĵ�ǰһ֡ͼ�񣬵���Ҫ���ʱ��������ͼ�񰴲�����λ�����ͬ��ε�ͼ��Ӧ�ó����ص���������򷵻ش����롣  _______v

        //2. �����������ƵԴ�᲻�������;�䶯�ֱ��ʵ����⣿   _______v

        //3. ������ƵԴ�Ĳ����� _______v

        //4. 
        class Transcoder{
		public:
			using DataFunc = std::function<void(uint8_t * data, size_t size)>;

			//ת���������ò���
			struct OutputConfig {
				int width = -1;
				int height = -1;
				int backgroundColor = 0xffffff;
				int framerate = -1;
				int bitrate = -1;
				NCodec::Type outCodecType = NCodec::Type::UNKNOWN;

				bool isConfigLegality() const {
					return (0 < width)
						&& (0 < height)
						&& (0 < framerate)
						&& (0 < bitrate)
						&& (NCodec::Type::UNKNOWN < outCodecType);
				}

				std::string dump() const {
					return fmt::format("\nwidth={},height={},backgroundColor=0x{:x}\nframerate={},bitrate={},outCodecType={}\n"
						, width, height, backgroundColor, framerate, bitrate, outCodecType);
				}
			};
        private:
            NLogger::shared             logger_ = nullptr;

            //����
            //RegionConfig��index��Ϊ��ɾ�Ĳ��������zOrder��Ϊ��ͼʱ������key��
            //channels��Ϊ��ͼʱ�ã���zOrder��Ϊkey����
            //numbers_��Ϊ��ɾ�Ĳ�ʱ�ã���index��Ϊ��ɾ�Ĳ������
            std::vector<Region::shared> channels_;
            std::map<int, Region::shared>   numbers_;

            OutputConfig				cfg_;
            AVFrame*                    bgFrame_ = nullptr;
            uint8_t*                    bgBuffer_ = nullptr;
			Region::ImgDrawParam        bgParam_;
            AVFrame*                    outFrame_ = nullptr;
            uint8_t*                    outBuffer_ = nullptr;
            //av_new_packet()   //����avpacket��buffer�ռ䡣
            //av_packet_alloc();
            AVPacket*                   inPacket_ = nullptr;
            AVPacket*                   outPacaket_ = nullptr;
            AVCodecContext*             imgCodecCtx_ = nullptr;
            DataFunc                    onEncodeFrame_ = nullptr;

        public:
            Transcoder(NLogger::shared logger) :logger_(logger) {}

            Transcoder(const std::string& name) :logger_(NLogger::Get(name)) {}

            virtual ~Transcoder() {
                close();
            }
        
			//����ת�����������ʱ�Ļص�����
            void output(const DataFunc& func){
                onEncodeFrame_ = func;
            }
            
            //��ʼ��ת����
            int init(const OutputConfig& cfg){
                if(isOpened()) {
					dbge(logger_, "transcoder already opened! cfg=[{}].", cfg_.dump());
                    return -1;
                }

                if(!cfg.isConfigLegality()) {
					dbge(logger_, "transcoder init param error! cfg=[{}].", cfg.dump());
                    return -2;
                }

				av_log_set_level(AV_LOG_QUIET);

                int ret = 0;

                cfg_ = cfg;

                ret = initBG();
                if(ret) {
                    return ret;
                }

                ret = initCodec();
                if(ret) {
                    return ret;
                }

                return 0;
            }
            
			//��ȡת��������
            const OutputConfig& getOutConfig() const {
                return cfg_;
            }

			//����һ·���·ͼ��index�����ظ���������ظ���������ͼ���޷����룬�����µ��÷���
            //ljh: change "setRegions" to "addRegions"
            //�߼���addRegions�������ӵ�һ���߶�·����
            //�����ӵ�������Ϊ���б��ʱ������ʧ�ܲ����س�ͻ��ţ����������κα������
            //@channels index must increase from 1��
            //@return 0 on success, otherwise negative error code or error number
            int addRegions(const std::vector<Region::RegionConfig>& channels){
                
                for(auto &i : channels) {
                    if(i.isRegionLegality()) {
                        auto search = numbers_.find(i.index);
                        if(search != numbers_.end()) {
                            dbge(logger_, "regions param illegal!, regions=[{}]/[{}].", i.index, i.dump());
                            return i.index;
                        }
                    } else {
                        dbge(logger_, "regions param illegal!, regions=[{}]/[{}].", i.index, i.dump());
                        return i.index;
                    }
                }
        
                for(auto &i : channels) {
                    Region::shared pr = std::make_shared<Region>(logger_);
                    pr->init(i);
                    channels_.push_back(pr);
                    numbers_[i.index] = pr;
                    dbgt(logger_, "regions joined successfully!, regions=[{}]/[{}].", i.index, i.dump());
                }

                //����z������������
                std::sort(channels_.begin(), channels_.end(), [](Region::shared a, Region::shared b) {
                    const Region::RegionConfig& cfga = a->getRegionCfg();
                    const Region::RegionConfig& cfgb = b->getRegionCfg();
                    return cfga.zOrder < cfgb.zOrder;   
                });

                return 0;
            }

			//�Ƴ�һ·ͼ
            int removeRegion(const Region::RegionConfig& channel) {
                for(auto it = channels_.begin(); it != channels_.end(); ++it) {
                    const Region::RegionConfig& ccfg = (*it)->getRegionCfg();
                    if(ccfg.index == channel.index) {
						(*it)->close();
                        channels_.erase(it);
                        break;
                    }
                }

                auto search = numbers_.find(channel.index);
                if(search == numbers_.end()) {
					dbgt(logger_, "No region found!, region=[{}]/[{}].", channel.index, channel.dump());
                }

				dbgt(logger_, "remove region !, region=[{}]/[{}].", channel.index, channel.dump());
				numbers_.erase(search);
				return 0;
            }

			//�Ƴ�����ͼ
            int removeAllRegions() {
                for(auto i : channels_) {
                    i->close();
                }

                channels_.clear();
                numbers_.clear();
				return 0;
            }

			//��������ʱ���ø÷�����������
            int input(int regionIndex, NVideoFrame * frame){
				if (!isSupportCodecType(frame->getCodecType())) {
					dbgi(logger_, "Unsupported codec type! index=[{}], Type=[{}].", regionIndex, NCodec::GetNameFor(frame->getCodecType()));
					return -1;
				}

                auto region = numbers_.find(regionIndex);
                if(region == numbers_.end()) {
                    dbgi(logger_, "Not found target region index! index=[{}].", regionIndex);
                    return -2;
                }

                return region->second->onInputFrame(frame);
            }

			const AVFrame* getDebug() {
				return outFrame_;
			}

			const AVFrame* getDebugFrame() {
				return channels_[0]->getSrcFrame();
			}

			//ת�벢�첽�����Ƶ��
			//ת�����Ƶ�������ɳ�ʼ��ת����ʱ����Ĳ�������
			//����transcode�������֡ͨ���ص��������
            int transcode() {
                if(!bgBuffer_
                    || !outPacaket_) {
                    return -1;
                }

                //�ú�ɫͼ�����ñ���
                int ret = brushYUV420P(bgParam_, bgFrame_);
                if(ret < 0) {
                    return -1;
                }

                //�����������������е�ͼ����뱳����
                for(auto i : channels_) {
                    if(i->isOpened()) {
                        ret = brushYUV420P(i->getDrawParam(), i->getDrawFrame());
                        if(ret < 0) {
                            return -2;
                        }
                    }
                }

                //����
                ret = encode();
                if(ret) {
                    if(ret < 0) {
                        return -3;
                    }
                    return 0;
                }

                //���
                if(onEncodeFrame_) {
                    onEncodeFrame_(outPacaket_->data, outPacaket_->size);
                }

                av_packet_unref(outPacaket_);
                return 0;
            }
        
			//ת�����Ƿ���
            bool isOpened() const {
                return nullptr != imgCodecCtx_;
            }

			//�ر�ת����
            void close() {

                if(bgFrame_) {
                    av_frame_free(&bgFrame_);
                }

                if(bgBuffer_) {
                    av_free(bgBuffer_);
                    bgBuffer_ = nullptr;
                }

                if(outFrame_) {
                    av_frame_free(&outFrame_);
                }

                if(outBuffer_) {
                    av_free(outBuffer_);
                    outBuffer_ = nullptr;
                }

                if(inPacket_) {
					av_packet_unref(inPacket_);
                }

                if(outPacaket_) {
					av_packet_unref(outPacaket_);
                }

                if(imgCodecCtx_) {
                    avcodec_close(imgCodecCtx_);
                    imgCodecCtx_ = nullptr;
                }

                if(onEncodeFrame_) {
                    onEncodeFrame_ = nullptr;
                }
            }

			//�Ƿ�֧�ִ���ı�������
			bool isSupportCodecType(const NCodec::Type typ) const {
				return typ == NCodec::Type::H264
					|| typ == NCodec::Type::VP8;
			}
        private:
			//����һ֡��Ƶ֡
            int encode() {
                if(!outFrame_
                || !outPacaket_) {
                    dbge(logger_, "Error during encoding, NULL parameter!");
                    return -1;
                }

                int ret = avcodec_send_frame(imgCodecCtx_, outFrame_);
                if (ret) {
                    if (AVERROR(EAGAIN) != ret) {
                        dbge(logger_, "Error sending original frame to encoder!");
                        return -2;
                    }
                    avcodec_receive_packet(imgCodecCtx_, outPacaket_);
                    av_packet_unref(outPacaket_);
                    avcodec_send_frame(imgCodecCtx_, outFrame_);
                }

                ret = avcodec_receive_packet(imgCodecCtx_, outPacaket_);
                if (ret) {
                    if (AVERROR(EAGAIN) != ret) {
                        dbge(logger_, "Error receiving encoded frame from encoder!");
                        return -3;
                    }
                    return 1;
                }

                return 0;
            }

			//����·�����뻭����
			int brushYUV420P(const Region::ImgDrawParam& imgConfig, const AVFrame* src) {

				if (!outBuffer_ || !outFrame_) {
					dbge(logger_, "An error occurred while painting, the parameter is NULL!");
					return -1;
				}
				else
					if (!src || !src->data) {
						return 0;
					}

				//�׿��汾
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

				//�򻯰汾
				for (int i = 0; i < imgConfig.drawSize.height; ++i) {
					//Y
					memcpy(outFrame_->data[0] + (i + imgConfig.imgInBgy) * outFrame_->width + imgConfig.imgInBgx
						, src->data[0] + (i + imgConfig.imgy) * src->width + imgConfig.imgx
						, imgConfig.drawSize.width);

					if (!(i % 2)) {
						//U
						memcpy(outFrame_->data[1] + (i + imgConfig.imgInBgy) / 2 * (outFrame_->width / 2) + imgConfig.imgInBgx / 2
							, src->data[1] + (i + imgConfig.imgy) / 2 * (src->width / 2) + imgConfig.imgx / 2
							, imgConfig.drawSize.width / 2);

						//V
						memcpy(outFrame_->data[2] + (i + imgConfig.imgInBgy) / 2 * (outFrame_->width / 2) + imgConfig.imgInBgx / 2
							, src->data[2] + (i + imgConfig.imgy) / 2 * (src->width / 2) + imgConfig.imgx / 2
							, imgConfig.drawSize.width / 2);
					}
				}

				return 0;
			}

			//��ʼ���������
            int initCodec() {

                if(!outFrame_) {
                    outFrame_ = av_frame_alloc();
                }
                
                if(outBuffer_) {
                    av_free(outBuffer_);
                }

                if(!inPacket_) {
                    inPacket_ = av_packet_alloc();
                    inPacket_->data = nullptr;
                    inPacket_->size = 0;
                }

                if(!outPacaket_) {
                    outPacaket_ = av_packet_alloc();
                }

                //out buffer
                outBuffer_ = (uint8_t*)av_malloc(av_image_get_buffer_size((AVPixelFormat)OUT_FF_FMT, cfg_.width, cfg_.height, 1));
                if(av_image_fill_arrays(outFrame_->data, outFrame_->linesize
                                    , outBuffer_, (AVPixelFormat)OUT_FF_FMT
                                    , cfg_.width, cfg_.height, 1) < 0) {
                    return -1;
                }

				outFrame_->width = cfg_.width;
				outFrame_->height = cfg_.height;
				outFrame_->format = (AVPixelFormat)OUT_FF_FMT;

                return initEncoder();
            }

			//��ʼ��������
            int initEncoder() {
                
                if(!imgCodecCtx_) {
                    int codecId = 0;
                    AVDictionary* param = nullptr;

                    codecId = convertFFCodecID(cfg_.outCodecType);
                    if(codecId < 0) {
                        dbge(logger_, "[encoder] not support codec type! NCodec::Type=[{}].", cfg_.outCodecType);
                        return -1;
                    }

                    AVCodec* pCodec = avcodec_find_encoder((AVCodecID)codecId);
                    if (!pCodec) {
                        dbge(logger_, "[encoder] Could not found video codec!  NCodec::Type=[{}].", cfg_.outCodecType);
                        return -2;
                    }

                    imgCodecCtx_ = avcodec_alloc_context3(pCodec);
                    if (!imgCodecCtx_) {
                        dbge(logger_, "[encoder] Could not allocate video codec context!  NCodec::Type=[{}].", cfg_.outCodecType);
                        return -3;
                    }

                    imgCodecCtx_->pix_fmt = (AVPixelFormat)OUT_FF_FMT;
                    imgCodecCtx_->width = cfg_.width;
                    imgCodecCtx_->height = cfg_.height;
                    imgCodecCtx_->bit_rate = cfg_.bitrate;
                    imgCodecCtx_->gop_size = 12;
                    imgCodecCtx_->flags |= AV_CODEC_FLAG_LOW_DELAY;

                    if(NCodec::Type::H264 == cfg_.outCodecType) {
                        paddingH264Codec(&param);
                    } else if (NCodec::Type::VP8 == cfg_.outCodecType) {
                        paddingVP8Codec(&param);
                    }

                    if (avcodec_open2(imgCodecCtx_, pCodec, &param) < 0) {
                        dbge(logger_, "[encoder] Failed to open encoder!  NCodec::Type=[{}].", cfg_.outCodecType);
                        return -4;
                    }

                    av_dict_free(&param);
                    param = nullptr;
                }

                return 0;
            }

			//���H264������
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

			//���VP8������
            void paddingVP8Codec(AVDictionary** param) {
                imgCodecCtx_->time_base.num = 1;
                imgCodecCtx_->time_base.den = 90000;

                imgCodecCtx_->qmin = 4;
                imgCodecCtx_->qmax = 63;
            }

            //init background
			//��ʼ������
            int initBG() {
                if(!bgFrame_) {
                    bgFrame_ = av_frame_alloc();
                }

                if(bgBuffer_) {
                    av_free(bgBuffer_);
                }

                //background buffer
                bgBuffer_ = (uint8_t*)av_malloc(av_image_get_buffer_size((AVPixelFormat)OUT_FF_FMT, cfg_.width, cfg_.height, 1));
                if(av_image_fill_arrays(bgFrame_->data, bgFrame_->linesize
                                    , bgBuffer_, (AVPixelFormat)OUT_FF_FMT
                                    , cfg_.width, cfg_.height, 1) < 0) {
                    dbge(logger_, "[encoder] init background error!");
                    return -1;
                }

                memset(bgFrame_->data[0], 0x00, cfg_.width * cfg_.height * 1 * sizeof(uint8_t));
                memset(bgFrame_->data[1], 0x80, cfg_.width * cfg_.height / 4 * sizeof(uint8_t));
                memset(bgFrame_->data[2], 0x80, cfg_.width * cfg_.height / 4 * sizeof(uint8_t));

                bgFrame_->width = cfg_.width;
                bgFrame_->height = cfg_.height;
                bgFrame_->format = (AVPixelFormat)OUT_FF_FMT;

                bgParam_.imgInBgx = 0;
                bgParam_.imgInBgy = 0;
                bgParam_.imgx = 0;
                bgParam_.imgy = 0;
                bgParam_.dstImgSize.width = cfg_.width;
                bgParam_.dstImgSize.height = cfg_.height;
                bgParam_.drawSize.width = cfg_.width;
                bgParam_.drawSize.height = cfg_.height;

                return 0;
            }

        };
    };
}

#endif //NVideoTranscoder_hpp
