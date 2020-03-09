
#ifndef NRegion_hpp
#define NRegion_hpp

namespace nmedia {
    namespace video{
        enum class ScalingMode {
            Unknown = 0,
            None,        //������
            AspectFit,   //ͬ�����䣬�ȱ����ţ�ĳ��������кڱ�
            AspectFill,  //ͬ����䣬�޺ڱߣ�ĳ���������ʾ���ݿ��ܱ��ü�
            Fill         //������䣬�ƻ�ԭ��Ƶ��������ԭʼ��Ƶ������һ��
        };
        
        struct Region {
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
            ScalingMode     scalinglMode = ScalingMode::AspectFit;
            
            bool valid() const {
                return (0 <= index)
                && (-1 < x)
                && (-1 < y)
                && (0 < width)
                && (0 < height)
                && (0 < zOrder)
                && (ScalingMode::Unknown < scalinglMode);
            }
        };
        
    }
}

#endif // NRegion_hpp

