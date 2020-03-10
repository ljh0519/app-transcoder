#ifndef __NERRORDEFINED_HPP__
#define __NERRORDEFINED_HPP__

#define INTERNAL_PARAM_NOT_VAILD	-1		//内部参数不可用
#define EXTERNAL_PARAM_NOT_VAILD	-2		//外部参数不可用
#define PARAM_EXISTS				-3		//参数已存在
#define PARAM_NOT_EXISTS			-4		//外部参数不存在
#define FAILED_INIT_DECODER			-5		//初始化解码器失败
#define FAILED_INIT_ENCODER			-6		//初始化编码器失败
#define FAILED_INIT_PARSER			-7		//初始化解析器失败
#define FAILED_INIT_YUVMIXER		-8		//初始化合流器失败
#define FAILED_INIT_CONVERTER		-9		//初始化分辨率调整工具失败
#define FAILED_FILL_BUFFER			-10		//填充缓冲区失败
#define ERROR_DECODE_VIDEO			-11		//解码视频出错
#define ERROR_ENCODE_VIDEO			-12		//编码视频出错

#define NOT_SUPPORT_CODEC_TYPE		-13		//不支持的编解码器类型
#define NOT_OPENED_REGION			-14		//区域region未开启
#define	ALREADY_OPENED_REGION		-15		//区域region已开启
#define NOT_OPENED_TRANSCODER		-16		//转码器未开启
#define ALREADY_OPENED_TRANSCODER	-17		//转码器已开启



#endif	//__NERRORDEFINED_HPP__