
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
            
            // �������ͼ��ߴ�
            virtual int outputConfig(int width, int height, uint32_t bkground_color) = 0;
            
            // ���һ��Region
            // ����ֵ >=0 ��ʾ�ɹ�������ֵΪRegion��index
            virtual int addRegion(const Region& r) = 0;
            
            // ����Region�б��ɵ�Region�б���ղ����ó��µ�Region�б�
            virtual int setRegions(const std::vector<Region>& v) = 0; 
            
            // ����1֡���ݵ�ָ��Region
            virtual int inputRegionFrame(int region_index,
                                         const AVFrame * frame) = 0;
            
            // ���1֡ͼ��
            virtual const AVFrame * outputFrame() = 0;
            
            static
            shared Create(const std::string& name);
        };
    }
}

#endif // YUVMixer_hpp

