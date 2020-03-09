
#ifndef NRegion_hpp
#define NRegion_hpp

namespace nmedia {
    namespace video{
        enum class ScalingMode {
            Unknown = 0,
            None,        //无缩放
            AspectFit,   //同比适配，等比缩放，某个方向会有黑边
            AspectFill,  //同比填充，无黑边，某个方向的显示内容可能被裁剪
            Fill         //满屏填充，破坏原视频比例，与原始视频比例不一致
        };
        
        struct Region {
            //       坐标定义
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
            int             zOrder = -1;        //zOrder数字越大，所在层次越高，越不会被遮挡。
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

