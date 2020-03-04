

#ifndef NMediaDefines_H
#define NMediaDefines_H


#include <iostream> 
#include <string.h>
#include <memory>

#ifdef _WIN32
#define strcasecmp stricmp
#endif

// see https://www.iana.org/assignments/media-types/media-types.xhtml
class NMedia{
public:
    enum Type {Audio=0,Video=1,Text=2,Application=3,Unknown=-1};
    static const char* GetNameFor(Type typ){
        switch (typ){
            case Audio:         return "audio";
            case Video:         return "video";
            case Text:          return "text";
            case Application:   return "application";
            default:            return "unknown";
        }
    }
    
    static Type GetMediaFor(const char* media){
        if    (strcasecmp(media,"audio")==0) return Audio;
        else if (strcasecmp(media,"video")==0) return Video;
        else if (strcasecmp(media,"text")==0) return Text;
        else if (strcasecmp(media,"application")==0) return Application;
        return Unknown;
    }
    
};

//typedef uint8_t NCodecNumType;
typedef int NCodecNumType;

typedef int NRTPPayloadType;
#define UNKNOWN_PAYLOAD_TYPE -1

class NCodec{
public:
    enum Type{
        UNKNOWN=-1,
        
        // audio codec
        PCMA=8,
        PCMU=0,
        GSM=3,
        G723=4,
        G722=9,
        G729=18,
        SPEEX16=117,
        AMR=118,
        TELEPHONE_EVENT=101,
        NELLY8=130,
        NELLY11=131,
        OPUS=98,
        AAC=97,
        
        // video codec
        JPEG=26,
        H261=31,
        H263_1996=34,
        H263_1998=103,
        MPEG4=104,
        H264=99,
        SORENSON=100,
        VP6=106,
        VP8=107,
        VP9=112,
        
        ULPFEC=108,
        FLEXFEC=113,
        RED=109,
        RTX=110,
        
        // text codec
        T140=102,
        T140RED=105
    };
    
public:
    static Type GetCodecForName(const char* codec){
        if    (strcasecmp(codec,"PCMA")==0) return PCMA;
        else if (strcasecmp(codec,"PCMU")==0) return PCMU;
        else if (strcasecmp(codec,"GSM")==0) return GSM;
        else if (strcasecmp(codec,"SPEEX16")==0) return SPEEX16;
        else if (strcasecmp(codec,"NELLY8")==0) return NELLY8;
        else if (strcasecmp(codec,"NELLY11")==0) return NELLY11;
        else if (strcasecmp(codec,"OPUS")==0) return OPUS;
        else if (strcasecmp(codec,"G722")==0) return G722;
        else if (strcasecmp(codec,"G723")==0) return G723;
        else if (strcasecmp(codec,"G729")==0) return G729;
        else if (strcasecmp(codec,"AAC")==0) return AAC;
        
        else if (strcasecmp(codec,"JPEG")==0) return JPEG;
        else if (strcasecmp(codec,"H261")==0) return H261;
        else if (strcasecmp(codec,"H263_1996")==0) return H263_1996;
        else if (strcasecmp(codec,"H263-1996")==0) return H263_1996;
        else if (strcasecmp(codec,"H263")==0) return H263_1996;
        else if (strcasecmp(codec,"H263P")==0) return H263_1998;
        else if (strcasecmp(codec,"H263_1998")==0) return H263_1998;
        else if (strcasecmp(codec,"H263-1998")==0) return H263_1998;
        else if (strcasecmp(codec,"MPEG4")==0) return MPEG4;
        else if (strcasecmp(codec,"H264")==0) return H264;
        else if (strcasecmp(codec,"SORENSON")==0) return SORENSON;
        else if (strcasecmp(codec,"VP6")==0) return VP6;
        else if (strcasecmp(codec,"VP8")==0) return VP8;
        else if (strcasecmp(codec,"VP9")==0) return VP9;
        else if (strcasecmp(codec,"ULPFEC")==0) return ULPFEC;
        else if (strcasecmp(codec,"FLEXFEC")==0) return FLEXFEC;
        else if (strcasecmp(codec,"RED")==0) return RED;
        else if (strcasecmp(codec,"RTX")==0) return RTX;
        return UNKNOWN;
    }
    
    static const char* GetNameFor(Type codec){
        switch (codec){
            case PCMA:    return "PCMA";
            case PCMU:    return "PCMU";
            case GSM:    return "GSM";
            case SPEEX16:    return "SPEEX16";
            case TELEPHONE_EVENT: return "telephone-event";
            case NELLY8:    return "NELLY8Khz";
            case NELLY11:    return "NELLY11Khz";
            case OPUS:    return "opus";
            case G722:    return "G722";
            case G723:    return "G723";
            case G729:    return "G729";
            case AAC:    return "AAC";
                
            case JPEG:    return "JPEG";
            case H261:    return "H261";
//            case H263_1996:    return "H263_1996";
//            case H263_1998:    return "H263_1998";
            case H263_1996:    return "H263";
            case H263_1998:    return "H263";
            case MPEG4:    return "MPEG4";
            case H264:    return "H264";
            case SORENSON:  return "SORENSON";
            case VP6:    return "VP6";
            case VP8:    return "VP8";
            case VP9:    return "VP9";
            case RED:    return "RED";
            case RTX:    return "RTX";
            case ULPFEC:    return "ULPFEC";
            case FLEXFEC:    return "flexfec-03";
                
            case T140:    return "T140";
            case T140RED:    return "T140RED";

            default:    return "unknown";
        }
    }
    
    static bool IsNative(Type codec){
        switch (codec){
            case PCMA:
            case PCMU:
            case GSM:
            case SPEEX16:
            case NELLY8:
            case NELLY11:
            case OPUS:
            case G722:
            case AAC:
                
            case H263_1996:
            case H263_1998:
            case MPEG4:
            case H264:
            case SORENSON:
            case VP6:
            case VP8:
            case VP9: return true;
            
            default:    return false;
        }
    }
    
    static uint32_t GetAudioClockRate(Type codec)
    {
        switch (codec)
        {
            case PCMA:    return 8000;
            case PCMU:    return 8000;
            case GSM:    return 8000;
            case SPEEX16:    return 16000;
            case NELLY8:    return 8000;
            case NELLY11:    return 11000;
            case OPUS:    return 48000;
            case G722:    return 16000;
            case AAC:    return 90000;
            default:    return 8000;
        }
    }
    
    static uint32_t GetClockRate(Type codec){
        if(GetMediaForCodec(codec) == NMedia::Audio){
            return GetAudioClockRate(codec);
        }else{
            return 90000;
        }
    }
    
    static NMedia::Type GetMediaForCodec(Type codec)
    {
        switch (codec)
        {
            case PCMA:
            case PCMU:
            case GSM:
            case SPEEX16:
            case NELLY8:
            case NELLY11:
            case OPUS:
            case G722:
            case AAC:
                return NMedia::Audio;
            case H263_1996:
            case H263_1998:
            case MPEG4:
            case H264:
            case SORENSON:
            case VP6:
            case VP8:
            case VP9:
            case RED:
            case RTX:
            case ULPFEC:
            case FLEXFEC:
                return NMedia::Video;
            case T140:
            case T140RED:
                return NMedia::Text;
            default:
                return NMedia::Unknown;
        }
    }
    
};// class NCodec

class NVideoSize{
public:
    NVideoSize():NVideoSize(0,0){}
    NVideoSize(int w, int h):width(w), height(h){}

    bool valid()const{
        return (width > 0 && height > 0);
    }
    
    bool operator > (const NVideoSize& other)const{
        return ((width*height) > (other.width*other.height));
    }

    bool operator < (const NVideoSize& other)const{
        return ((width*height) < (other.width*other.height));
    }
    
    bool operator == (const NVideoSize& other)const{
        return ((width*height) == (other.width*other.height));
    }
    
    bool operator != (const NVideoSize& other)const{
        return ((width*height) != (other.width*other.height));
    }
    
    friend inline std::ostream& operator<<(std::ostream& out, const NVideoSize& obj){
        out << obj.width << "x" << obj.height;
        return out;
    }
    
    
    int width;
    int height;
};


class TrackFileReader{
public:
    using shared = std::shared_ptr<TrackFileReader>;
    
public:
    virtual ~TrackFileReader(){}
    
    virtual int64_t time() = 0;
    
    // return >0, bytes read
    // return =0, reach file end
    // return <0, error
    virtual unsigned int next() = 0;
    
    virtual uint8_t * data() = 0;
    
    virtual void close() = 0;
};

#endif /* NMediaDefines_H */


