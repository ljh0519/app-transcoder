
#ifndef YUVMixer_hpp
#define YUVMixer_hpp

#include <memory> // std::shared_ptr
#include <vector>
#include "NRegion.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
};

namespace nmedia {
    namespace video{
        class YUVMixer{
        public:
            using shared = std::shared_ptr<YUVMixer>;
            
            ~YUVMixer(){}
            
            // 设置输出图像尺寸
			// 0 : 成功
			// EXTERNAL_PARAM_NOT_VAILD : 输入参数不可用
			// FAILED_FILL_BUFFER : 填充缓冲区失败
			// INTERNAL_PARAM_NOT_VAILD : 背景缓冲区为空
            virtual int outputConfig(int width, int height, uint32_t bkground_color) = 0;
            
            // 添加一个Region
			// region.index : 成功
			// EXTERNAL_PARAM_NOT_VAILD ： region参数不可用
			// PARAM_EXISTS ： region.index重复
            virtual int addRegion(const RegionConfig& r) = 0;
            
            // 设置Region列表，旧的Region列表被清空并设置成新的Region列表
			// 如果新的region列表中有region参数不正确，则不会有任何region被成功设置
			// 0 : 成功
			// PARAM_EXISTS ： index重复
			// region.index : region参数非法，返回非法index
            virtual int setRegions(const std::vector<RegionConfig>& v) = 0;
            
            // 输入1帧数据到指定Region
			// 0 : 成功
			// EXTERNAL_PARAM_NOT_VAILD : 输入frame为空
			// PARAM_NOT_EXISTS : 输入index不存在
			// FAILED_INIT_CONVERTER : 分辨率转换失败
            virtual int inputRegionFrame(int region_index,
                                         const AVFrame * frame) = 0;
            
            // 输出1帧合成后的图像
			// 非空 : 包含合成图像的AVFrame*数据结构
			// nullptr : 输出缓冲区为NULL
            virtual const AVFrame * outputFrame() = 0;
            
			//创建一个yuv mixer实例
            static
            shared Create(const std::string& name);
        };
    }
}

#endif // YUVMixer_hpp

