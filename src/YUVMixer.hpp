
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
            virtual int outputConfig(int width, int height, uint32_t bkground_color) = 0;
            
            // 添加一个Region
            // 返回值 >=0 表示成功，返回值为Region的index
            virtual int addRegion(const Region& r) = 0;
            
            // 设置Region列表，旧的Region列表被清空并设置成新的Region列表
            virtual int setRegions(const std::vector<Region>& v) = 0; 
            
            // 输入1帧数据到指定Region
            virtual int inputRegionFrame(int region_index,
                                         const AVFrame * frame) = 0;
            
            // 输出1帧图像
            virtual const AVFrame * outputFrame() = 0;
            
            static
            shared Create(const std::string& name);
        };
    }
}

#endif // YUVMixer_hpp

