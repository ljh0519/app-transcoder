
#include "YUVMixer.hpp"

#include "NMediaBasic.hpp"
extern "C" {
#include "libswscale/swscale.h"
};

namespace nmedia {
    namespace video{
        class YUVMixerImpl : public YUVMixer{
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
                Region                  region_;
                ImgDrawParam            imgConfig_;
                AVPacket*               imgPacket_ = nullptr;
                AVFrame*                inFrame_ = nullptr;
                AVFrame*                swsFrame_ = nullptr;
                uint8_t*                swsBuf_ = nullptr;
                SwsContext*             imgConvertCtx_ = nullptr;
                
                //获取当前适应模式下应缩放成的分辨率
                inline void getDstResolution(int srcWidth, int srcHeight) {
                    
                    if(ScalingMode::None == region_.scalinglMode) {
                        imgConfig_.dstImgSize.width = srcWidth;
                        imgConfig_.dstImgSize.height = srcHeight;
                    } else if(ScalingMode::AspectFit == region_.scalinglMode) {      //TODO:这个模式编写的有没有问题
                        
                        //以一条边为基准缩放
                        imgConfig_.dstImgSize.width = region_.width;
                        imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
                        if(imgConfig_.dstImgSize.height > region_.height) {
                            //以另一条边为基准缩放
                            imgConfig_.dstImgSize.height = region_.height;
                            imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
                        }
                        
                    } else if(ScalingMode::AspectFill == region_.scalinglMode) {
                        
                        //以一条边为基准缩放
                        imgConfig_.dstImgSize.width = region_.width;
                        imgConfig_.dstImgSize.height = imgConfig_.dstImgSize.width * srcHeight / srcWidth;
                        if(imgConfig_.dstImgSize.height < region_.height) {
                            //以另一条边为基准缩放
                            imgConfig_.dstImgSize.height = region_.height;
                            imgConfig_.dstImgSize.width = srcWidth * imgConfig_.dstImgSize.height / srcHeight;
                        }
                        
                    } else if(ScalingMode::Fill == region_.scalinglMode) {
                        imgConfig_.dstImgSize.width = region_.width;
                        imgConfig_.dstImgSize.height = region_.height;
                    }
                }
            };
            
            
        public:
            
            ~YUVMixerImpl(){}
            
            // 设置输出图像尺寸
            virtual int outputConfig(int width, int height, uint32_t bkground_color) override{
                return -1;
            }
            
            // 添加一个Region
            // 返回值 >=0 表示成功，返回值为Region的index
            virtual int addRegion(const Region& r) override{
                return -1;
            }
            
            // 设置Region列表，旧的Region列表被清空并设置成新的Region列表
            virtual int setRegions(const std::vector<Region>& v) override{
                return -1;
            }
            
            // 输入1帧数据到指定Region
            virtual int inputRegionFrame(int region_index,
                                         const AVFrame * frame) override{
                return -1;
            }
            
            // 输出1帧图像
            virtual const AVFrame * outputFrame() override{
                return nullptr;
            }
            
        };
        
        YUVMixer::shared YUVMixer::Create(){
            return std::make_shared<YUVMixerImpl>();
        }
    }
}
