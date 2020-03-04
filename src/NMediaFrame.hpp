
#ifndef NMediaFrame_hpp
#define NMediaFrame_hpp

#include <stdio.h>
#include <deque>
#include <cassert>
#include "NPool.hpp"
#include "NMediaBasic.hpp"

// mutable data
struct NMData{
    uint8_t *           data;
    size_t              size;
    NMData(uint8_t * d, size_t sz)
    : data(d), size(sz){ }
    
    NMData():NMData(nullptr, 0){}
};

// immutable data
struct NIData{
    const uint8_t *     data;
    size_t              size;
    
    NIData(const uint8_t * d, size_t sz)
    : data(d), size(sz){ }
    
    NIData():NIData(nullptr, 0){}
};

template <typename T>
class NBuffer {
public:
    static const size_t DEFAULT_CAPACITY = 128;
    
public:
    
    // An empty buffer
    NBuffer() : size_(0), capacity_(0), data_(nullptr) {
        
    }
    
    // Copy size and contents of an existing buffer
    NBuffer(const NBuffer& buf)
    //: NBuffer(buf.data(), buf.size(), buf.capacity())
    {
        SetData(buf);
    }
    
    NBuffer(const T* data, size_t size, size_t capacity)
    : NBuffer(size, capacity) {
        std::memcpy(data_.get(), data, size);
    }
    
    // Move contents from an existing buffer
    NBuffer(NBuffer&& buf)
//    : size_(buf.size_)
//    , capacity_(buf.capacity_)
//    , data_(std::move(buf.data_))
//    , pos_(buf.pos_)
    {
        MoveData(buf);
    }
    
    NBuffer(size_t capacity) : NBuffer(capacity, DEFAULT_CAPACITY) {
    }
    
    NBuffer(size_t capacity, size_t step_capacity)
    : size_(0)
    , capacity_(capacity)
    , data_(new T[capacity_])
    , capacity_step_(step_capacity)
    {

    }
    
    
    // Note: The destructor works even if the buffer has been moved from.
    virtual ~NBuffer(){
        
    }

    NBuffer& operator=(NBuffer&& buf) {
        MoveData(buf);
        return *this;
    }
    
    const T* data() const {
        return (data_.get()+pos_);
    }
    
    
    T* data() {
        return (data_.get()+pos_);
    }
    
    size_t size() const {
        return size_;
    }
    
//    size_t capacity() const {
//        return capacity_-pos_;
//    }
    
    NBuffer& operator=(const NBuffer& buf) {
        SetData(buf);
        return *this;
    }

    bool operator==(const NBuffer& buf) const {
        return size_ == buf.size() && memcmp(data(), buf.data(), size_) == 0;
    }
    
    bool operator!=(const NBuffer& buf) const { return !(*this == buf); }
    
    T& operator[](size_t index) {
        return data()[index];
    }
    
    T operator[](size_t index) const {
        return data()[index];
    }
    
    void setData(const T* data, size_t size) {
        size_ = 0;
        appendData(data, size);
    }
    
    void setData(const NBuffer& buf) {
        if (&buf != this){
            pos_ = buf.pos_;
            size_ = 0;
            AppendData(buf.data(), buf.size());
            //SetData(buf.data(), buf.size());
        }
    }
    
    void appendData(const T* d, size_t size) {
        const size_t new_size = size_ + size;
        EnsureCapacity(pos_, new_size);
        std::memcpy(data() + size_, d, size);
        size_ = new_size;
    }

    void prepend(size_t reserved){
        if(pos_ >= reserved){
            return;
        }
        EnsureCapacity(reserved, size_);
    }

    void prepend(const T* data, size_t size){
        prepend(size);
        pos_ -= size;
        std::memcpy(data_.get() + pos_, data, size);
        size_ += size;
    }
    
    void moveData(NBuffer&& buf) {
        size_ = buf.size_;
        capacity_ = buf.capacity_;
        data_ = std::move(buf.data_);
        pos_ = buf.pos_;
    }
    
//    void SetSize(size_t size) {
//        EnsureCapacity(size);
//        size_ = size;
//    }


    
    void clear() {
        size_ = 0;
    }
    
    friend void swap(NBuffer& a, NBuffer& b) {
        using std::swap;
        swap(a.size_, b.size_);
        swap(a.capacity_, b.capacity_);
        swap(a.data_, b.data_);
        swap(a.pos_, b.pos_);
    }
    
protected:
//    void EnsureCapacity(size_t capacity) {
//        if (capacity <= capacity_){
//            return;
//        }
//
//        const size_t multi = (capacity + capacity_step_-1) / capacity_step_ ;
//        capacity = multi * capacity_step_;
//
//        std::unique_ptr<T[]> new_data(new T[capacity]);
//        if(size_ > 0){
//            std::memcpy(new_data.get()+pos_, data(), size_);
//        }
//        data_ = std::move(new_data);
//        capacity_ = capacity;
//    }
    
    void EnsureCapacity(size_t new_pos, size_t new_size){
        size_t new_capacity = new_pos + new_size;
        if (new_capacity <= capacity_){
            if(new_pos != pos_){
                if(size_ > 0){
                    std::memmove(data_.get()+new_pos, data_.get()+pos_, size_);
                }
                pos_ = new_pos;
            }
            return;
        }
        
        const size_t multi = (new_capacity + capacity_step_-1) / capacity_step_ ;
        new_capacity = multi * capacity_step_;
        
        std::unique_ptr<T[]> new_data(new T[new_capacity]);
        if(size_ > 0){
            std::memcpy(new_data.get()+new_pos, data(), size_);
        }
        data_ = std::move(new_data);
        capacity_ = new_capacity;
        pos_ = new_pos;
    }

protected:
    size_t size_;
    size_t capacity_;
    std::unique_ptr<T[]> data_;
    size_t capacity_step_ = DEFAULT_CAPACITY;
    size_t pos_ = 0;
};


class NMediaFrame : public NBuffer<uint8_t>{
public:
    using Unique = NObjectPool<NMediaFrame>::Unique;
    static inline Unique MakeNullPtr(){
        return NObjectPool<NMediaFrame>::MakeNullPtr();
    }
public:

    NMediaFrame(size_t capacity, size_t step_capacity=1024
                , NMedia::Type mtype = NMedia::Unknown, NCodec::Type ctype = NCodec::UNKNOWN)
    :NBuffer(capacity),  mediaType_(mtype), codecType_(ctype) {
    }
    
    virtual ~NMediaFrame(){
    }
    
    void setPts(int64_t t){
        pts_ = t; 
    }
    
    int64_t getPts()const{
        return pts_;
    }
    
    void setGts(int64_t t){
        gts_ = t;
    }
    
    int64_t getGts()const{
        return gts_;
    }
    
    void setCodecType(NCodec::Type codec_type, NMedia::Type media_type){
        codecType_ = codec_type;
        mediaType_ = media_type;
    }
    
    NCodec::Type getCodecType() const{
        return codecType_;
    }
    
    NMedia::Type getMediaType() const{
        return mediaType_;
    }
    
protected:
    NMedia::Type    mediaType_;
    NCodec::Type    codecType_;
    int64_t         pts_ = -1; // Presentation timestamp
    int64_t         gts_ = -1; // Generation time

};

//class NMediaFramePool {
//public:
//    NMediaFramePool(NCodec::Type codec_type, size_t start_capacity=1)
//    : codecType_(codec_type), mediaType_(NCodec::GetMediaForCodec(codec_type)){
//        if(mediaType_ == NMedia::Video){
//            frameStartSize_ = 30*1024;
//            frameStepSize_ = 30*1024;
//        }else{
//            frameStartSize_ = 1700;
//            frameStepSize_ = 1024;
//        }
//        for(size_t n = 0; n < start_capacity; ++n){
//            freeQ_.emplace_back(new NMediaFrame(frameStartSize_, frameStepSize_, mediaType_, codecType_));
//        }
//
//    }
//
//    virtual ~NMediaFramePool(){
//        for(auto& frame : freeQ_){
//            delete frame;
//        }
//    }
//
//    virtual NMediaFrame * alloc() {
//        if(freeQ_.empty()){
//            return new NMediaFrame(frameStartSize_, frameStepSize_, mediaType_, codecType_);
//        }else{
//            NMediaFrame * frame = freeQ_.back();
//            freeQ_.pop_back();
//            return frame;
//        }
//    }
//
//    virtual void free(NMediaFrame *frame){
//        freeQ_.emplace_back(frame);
//    }
//
//protected:
//    NCodec::Type codecType_;
//    NMedia::Type mediaType_ ;
//    std::deque<NMediaFrame *> freeQ_;
//    size_t frameStartSize_;
//    size_t frameStepSize_;
//};


class NAudioFrame : public NMediaFrame{
public:
    static const size_t kStartCapacity = 1700;
    static const size_t kStepCapacity = 1*1024;
    class Pool : public NPool<NAudioFrame, NMediaFrame>{
        
    };
public:
    NAudioFrame():NAudioFrame(NCodec::UNKNOWN){}
    NAudioFrame(NCodec::Type ctype) : NMediaFrame(kStartCapacity, kStepCapacity, NMedia::Audio){
        
    }
};

class NVideoFrame : public NMediaFrame{
public:
    static const size_t kStartCapacity = 30*1024;
    static const size_t kStepCapacity = 30*1024;
    class Pool : public NPool<NVideoFrame, NMediaFrame>{
        
    };
private:
    NVideoSize size_;
public:
    NVideoFrame():NVideoFrame(NCodec::UNKNOWN){}
    NVideoFrame(NCodec::Type ctype) : NMediaFrame(kStartCapacity, kStepCapacity, NMedia::Video){
        
    }
    
    virtual bool isKeyframe(){
        return false;
    };
    
    virtual const NVideoSize& videoSize() {
        return size_;
    };
    
    virtual int64_t pictureId() {
        return -1;
    };

    void setSize(const NVideoSize& size) {
        size_ = size;
    }
    
    // return true if first unit
    bool first()const{
        return true;
    }
    
    // return true if last unit (mark)
    bool last()const{
        return true;
    }
    
    // return true if contains the whole frame
    // return false if only contains 1 nalu for H264
    bool full()const{
        return true;
    }
    
    virtual size_t units() const{
        return 1;
    }
    
    virtual void unit(size_t index, NIData * d) const{
        assert(index == 0);
        d->data = unitPtr(index);
        d->size = unitSize(index);
    }
    
    virtual const uint8_t* unitPtr(size_t index) const{
        assert(index == 0);
        return NBuffer::data();
    }
    
    virtual size_t unitSize(size_t index) const{
        assert(index == 0);
        return NBuffer::size();
    }
    
};


#endif /* NMediaFrame_hpp */

