

#ifndef NPool_hpp
#define NPool_hpp

#include <stdio.h>
#include <cstring>
#include <stack>
#include <functional>
#include <memory>
#include <vector>

// see https://swarminglogic.com/jotting/2015_05_smartpool
template <class T, class D = std::default_delete<T>>
class NObjectPool
{
private:
    using PoolType = NObjectPool<T, D>;
    struct ReturnToPool_Deleter {
        explicit ReturnToPool_Deleter(std::weak_ptr<PoolType* > pool)
        : pool_(pool) {}
        
        void operator()(T* ptr) {
            if (auto pool_ptr = pool_.lock()){
                (*pool_ptr.get())->put(std::unique_ptr<T, D>{ptr});
            }else{
                D{}(ptr);
            }
            
        }
    private:
        std::weak_ptr<PoolType* > pool_;
    };
    
public:
    using Unique = std::unique_ptr<T, ReturnToPool_Deleter >;
    using CreateType = std::function<T *()>;
    
    static inline Unique MakeNullPtr(){
        return Unique(nullptr,
                      ReturnToPool_Deleter{
                          std::weak_ptr<PoolType*>{std::shared_ptr<PoolType* >(nullptr)}});
    }
public:
    NObjectPool()
    : this_ptr_(new PoolType*(this)) {}
    NObjectPool(const CreateType& creator)
    : this_ptr_(new PoolType*(this)), creator_(creator) {}
    
    virtual ~NObjectPool(){}
    
    void put(std::unique_ptr<T, D> t) {
        pool_.push(std::move(t));
    }
    
    Unique get() {
        if (pool_.empty()){
            auto t = std::unique_ptr<T>(creator_());
            pool_.push(std::move(t));
        }
        
        Unique tmp(pool_.top().release(),
                   ReturnToPool_Deleter{
                       std::weak_ptr<PoolType*>{this_ptr_}});
        pool_.pop();
        return std::move(tmp);
    }
    
    bool empty() const {
        return pool_.empty();
    }
    
    size_t size() const {
        return pool_.size();
    }
    
private:
    std::shared_ptr<PoolType* > this_ptr_;
    std::stack<std::unique_ptr<T, D> > pool_;
    const CreateType creator_ = []()->T *{
        return new T();
    };
};

template <class T, class BaseType=T>
class NPool : public NObjectPool<BaseType>{
public:
    using Parent = NObjectPool<BaseType>;
public:
    NPool():NObjectPool<BaseType>([]()->BaseType*{
        return new T();
    }){};
    
    NPool(typename NObjectPool<BaseType>::CreateType creator):NObjectPool<BaseType>(creator){}
};






// refer https://android.googlesource.com/platform/libcore/+/2496a68/luni/src/main/java/java/nio/Buffer.java
template <typename T>
class NIOBuffer {
private:
    using SizeType = unsigned int;
    /**
     * <code>UNSET_MARK</code> means the mark has not been set.
     */
    static const unsigned int UNSET_MARK = -1;
    
    /**
     * The capacity of this buffer, which never changes.
     */
    // SizeType capacity;
    
    /**
     * <code>limit - 1</code> is the last element that can be read or written.
     * Limit must be no less than zero and no greater than <code>capacity</code>.
     */
    SizeType limit_;
    
    /**
     * Mark is where position will be set when <code>reset()</code> is called.
     * Mark is not set by default. Mark is always no less than zero and no
     * greater than <code>position</code>.
     */
    SizeType mark_ = UNSET_MARK;
    
    /**
     * The current position of this buffer. Position is always no less than zero
     * and no greater than <code>limit</code>.
     */
    SizeType position_ = 0;
    
    /**
     * The log base 2 of the element size of this buffer.  Each typed subclass
     * (ByteBuffer, CharBuffer, etc.) is responsible for initializing this
     * value.  The value is used by JNI code in frameworks/base/ to avoid the
     * need for costly 'instanceof' tests.
     */
    //SizeType _elementSizeShift;
    
    /**
     * For direct buffers, the effective address of the data; zero otherwise.
     * This is set in the constructor.
     * TODO: make this final at the cost of loads of extra constructors? [how many?]
     */
    // long effectiveDirectAddress;
    
    /**
     * For direct buffers, the underlying MemoryBlock; null otherwise.
     */
    std::vector<T> block_;
    
public:
    NIOBuffer(unsigned int cap) : limit_(cap), block_(cap){
        
    }
    
    virtual ~NIOBuffer(){
        
    }
    
    /**
     * Returns the array that backs this buffer (optional operation).
     * The returned value is the actual array, not a copy, so modifications
     * to the array write through to the buffer.
     *
     * <p>Subclasses should override this method with a covariant return type
     * to provide the exact type of the array.
     *
     * <p>Use {@code hasArray} to ensure this method won't throw.
     * (A separate call to {@code isReadOnly} is not necessary.)
     *
     * @return the array
     * @throws ReadOnlyBufferException if the buffer is read-only
     *         UnsupportedOperationException if the buffer does not expose an array
     * @since 1.6
     */
    //public abstract Object array();
    
    
    /**
     * Returns the offset into the array returned by {@code array} of the first
     * element of the buffer (optional operation). The backing array (if there is one)
     * is not necessarily the same size as the buffer, and position 0 in the buffer is
     * not necessarily the 0th element in the array. Use
     * {@code buffer.array()[offset + buffer.arrayOffset()} to access element {@code offset}
     * in {@code buffer}.
     *
     * <p>Use {@code hasArray} to ensure this method won't throw.
     * (A separate call to {@code isReadOnly} is not necessary.)
     *
     * @return the offset
     * @throws ReadOnlyBufferException if the buffer is read-only
     *         UnsupportedOperationException if the buffer does not expose an array
     * @since 1.6
     */
    //public abstract int arrayOffset();
    
    /**
     * Returns the capacity of this buffer.
     *
     * @return the number of elements that are contained in this buffer.
     */
    SizeType capacity() const{
        return block_.size();
    }
    
    /**
     * Used for the scalar get/put operations.
     */
    //    void checkIndex(int index) {
    //        if (index < 0 || index >= limit) {
    //            throw new IndexOutOfBoundsException("index=" + index + ", limit=" + limit);
    //        }
    //    }
    
    
    /**
     * Used for the ByteBuffer operations that get types larger than a byte.
     */
    //    void checkIndex(int index, int sizeOfType) {
    //        if (index < 0 || index > limit - sizeOfType) {
    //            throw new IndexOutOfBoundsException("index=" + index + ", limit=" + limit +
    //                                                ", size of type=" + sizeOfType);
    //        }
    //    }
    //    int checkGetBounds(int bytesPerElement, int length, int offset, int count) {
    //        int byteCount = bytesPerElement * count;
    //        if ((offset | count) < 0 || offset > length || length - offset < count) {
    //            throw new IndexOutOfBoundsException("offset=" + offset +
    //                                                ", count=" + count + ", length=" + length);
    //        }
    //        if (byteCount > remaining()) {
    //            throw new BufferUnderflowException();
    //        }
    //        return byteCount;
    //    }
    //    int checkPutBounds(int bytesPerElement, int length, int offset, int count) {
    //        int byteCount = bytesPerElement * count;
    //        if ((offset | count) < 0 || offset > length || length - offset < count) {
    //            throw new IndexOutOfBoundsException("offset=" + offset +
    //                                                ", count=" + count + ", length=" + length);
    //        }
    //        if (byteCount > remaining()) {
    //            throw new BufferOverflowException();
    //        }
    //        if (isReadOnly()) {
    //            throw new ReadOnlyBufferException();
    //        }
    //        return byteCount;
    //    }
    //    void checkStartEndRemaining(int start, int end) {
    //        if (end < start || start < 0 || end > remaining()) {
    //            throw new IndexOutOfBoundsException("start=" + start + ", end=" + end +
    //                                                ", remaining()=" + remaining());
    //        }
    //    }
    
    /**
     * Clears this buffer.
     * <p>
     * While the content of this buffer is not changed, the following internal
     * changes take place: the current position is reset back to the start of
     * the buffer, the value of the buffer limit is made equal to the capacity
     * and mark is cleared.
     *
     * @return this buffer.
     */
    NIOBuffer * clear() {
        position_ = 0;
        mark_ = UNSET_MARK;
        limit_ = block_.size();
        return this;
    }
    
    /**
     * Flips this buffer.
     * <p>
     * The limit is set to the current position, then the position is set to
     * zero, and the mark is cleared.
     * <p>
     * The content of this buffer is not changed.
     *
     * @return this buffer.
     */
    NIOBuffer * flip() {
        limit_ = position_;
        position_ = 0;
        mark_ = UNSET_MARK;
        return this;
    }
    
    /**
     * Returns true if {@code array} and {@code arrayOffset} won't throw. This method does not
     * return true for buffers not backed by arrays because the other methods would throw
     * {@code UnsupportedOperationException}, nor does it return true for buffers backed by
     * read-only arrays, because the other methods would throw {@code ReadOnlyBufferException}.
     * @since 1.6
     */
    //public abstract boolean hasArray();
    
    /**
     * Indicates if there are elements remaining in this buffer, that is if
     * {@code position < limit}.
     *
     * @return {@code true} if there are elements remaining in this buffer,
     *         {@code false} otherwise.
     */
    bool hasRemaining() const{
        return position_ < limit_;
    }
    
    /**
     * Returns true if this is a direct buffer.
     * @since 1.6
     */
    //public abstract boolean isDirect();
    
    /**
     * Indicates whether this buffer is read-only.
     *
     * @return {@code true} if this buffer is read-only, {@code false}
     *         otherwise.
     */
    //    public abstract boolean isReadOnly();
    //    final void checkWritable() {
    //        if (isReadOnly()) {
    //            throw new IllegalArgumentException("Read-only buffer");
    //        }
    //    }
    
    /**
     * Returns the limit of this buffer.
     *
     * @return the limit of this buffer.
     */
    SizeType limit() const{
        return limit_;
    }
    
    /**
     * Sets the limit of this buffer.
     * <p>
     * If the current position in the buffer is in excess of
     * <code>newLimit</code> then, on returning from this call, it will have
     * been adjusted to be equivalent to <code>newLimit</code>. If the mark
     * is set and is greater than the new limit, then it is cleared.
     *
     * @param newLimit
     *            the new limit, must not be negative and not greater than
     *            capacity.
     * @return this buffer.
     * @exception IllegalArgumentException
     *                if <code>newLimit</code> is invalid.
     */
    //    public final Buffer limit(int newLimit) {
    //        if (newLimit < 0 || newLimit > capacity) {
    //            throw new IllegalArgumentException("Bad limit (capacity " + capacity + "): " + newLimit);
    //        }
    //        limit = newLimit;
    //        if (position > newLimit) {
    //            position = newLimit;
    //        }
    //        if ((mark != UNSET_MARK) && (mark > newLimit)) {
    //            mark = UNSET_MARK;
    //        }
    //        return this;
    //    }
    
    
    /**
     * Marks the current position, so that the position may return to this point
     * later by calling <code>reset()</code>.
     *
     * @return this buffer.
     */
    NIOBuffer * mark() {
        mark_ = position_;
        return this;
    }
    
    /**
     * Returns the position of this buffer.
     *
     * @return the value of this buffer's current position.
     */
    SizeType position() const {
        return position_;
    }
    
    /**
     * Sets the position of this buffer.
     * <p>
     * If the mark is set and it is greater than the new position, then it is
     * cleared.
     *
     * @param newPosition
     *            the new position, must be not negative and not greater than
     *            limit.
     * @return this buffer.
     * @exception IllegalArgumentException
     *                if <code>newPosition</code> is invalid.
     */
    //    NIOBuffer * position(int newPosition) {
    //        positionImpl(newPosition);
    //        return this;
    //    }
    //    void positionImpl(int newPosition) {
    ////        if (newPosition < 0 || newPosition > limit_) {
    ////            throw new IllegalArgumentException("Bad position (limit " + limit + "): " + newPosition);
    ////        }
    //        position_ = newPosition;
    //        if ((mark_ != UNSET_MARK) && (mark_ > position_)) {
    //            mark_ = UNSET_MARK;
    //        }
    //    }
    
    /**
     * Returns the number of remaining elements in this buffer, that is
     * {@code limit - position}.
     *
     * @return the number of remaining elements in this buffer.
     */
    SizeType remaining() const {
        return limit_ - position_;
    }
    
    /**
     * Resets the position of this buffer to the <code>mark</code>.
     *
     * @return this buffer.
     * @exception InvalidMarkException
     *                if the mark is not set.
     */
    NIOBuffer * reset() {
        //        if (mark == UNSET_MARK) {
        //            throw new InvalidMarkException("Mark not set");
        //        }
        //        position = mark;
        //        return this;
        if(mark_ != UNSET_MARK){
            position_ = mark_;
        }
        return this;
    }
    
    /**
     * Rewinds this buffer.
     * <p>
     * The position is set to zero, and the mark is cleared. The content of this
     * buffer is not changed.
     *
     * @return this buffer.
     */
    NIOBuffer * rewind() {
        position_ = 0;
        mark_ = UNSET_MARK;
        return this;
    }
    
    //    /**
    //     * Returns a string describing this buffer.
    //     */
    //    @Override public String toString() {
    //        return getClass().getName() +
    //        "[position=" + position + ",limit=" + limit + ",capacity=" + capacity + "]";
    //    }
    
    T * next(){
        return &block_[position_];
    }
    
    SizeType get(NIOBuffer& dst) {
        SizeType n = get(dst.next(), dst.remaining());
        dst.position_ += n;
        return n;
    }
    
    SizeType get(T * dst_ptr, SizeType dst_size) {
        SizeType n = std::min(remaining(), dst_size);
        if (n > 0) {
            std::memcpy(&dst_ptr[0], &block_[position_], n*sizeof(T));
            position_ += n;
        }
        return n;
    }
    
    SizeType put(NIOBuffer& src) {
        SizeType n = get(src.next(), src.remaining());
        src.position_ += n;
        return n;
    }
    
    SizeType put(const T * src_ptr, SizeType src_size) {
        SizeType n = std::min(remaining(), src_size);
        if (n > 0) {
            std::memcpy(&block_[position_], src_ptr, n*sizeof(T));
            position_ += n;
        }
        return n;
    }
    
    void forward(SizeType delta_pos){
        position_ = limitIn(position_ + delta_pos);
    }
    
private:
    SizeType limitIn(SizeType pos){
        if(pos >= 0 && pos <= limit_){
            return pos;
        }else if(pos > limit_){
            return limit_;
        }else{
            return 0;
        }
    }
    
};

class NIOByteBuffer : public NIOBuffer<uint8_t>{
public:
    //using Pool = NPool<NIOByteBuffer>;
    class Pool : public NPool<NIOByteBuffer>{
    public:
        Pool(size_t cap) : NPool<NIOByteBuffer>([cap]()->NIOByteBuffer*{
            return new NIOByteBuffer(cap);
        }){
            
        }
        
        Pool::Unique alloc(){
            auto buf = this->get();
            buf->clear();
            return buf;
        }
    };
    
    using unique = Pool::Unique;
    using shared = std::shared_ptr<NIOByteBuffer>;
public:
    NIOByteBuffer(size_t cap) : NIOBuffer<uint8_t>(cap){
        
    }
    
    virtual ~NIOByteBuffer(){
        
    }
};

class NIOByteBufferQ : public std::deque<NIOByteBuffer::unique>{
private:
    using Parent = std::deque<NIOByteBuffer::unique>;
    
public:
    NIOByteBufferQ(){}
    
    virtual ~NIOByteBufferQ(){}
    
    using Parent::size;
    using Parent::front;
    using Parent::pop_front;
    
    void append(NIOByteBuffer::Pool& pool, const uint8_t * data, size_t size){
        size_t num = 0;
        if(!this->empty()){
            num += this->back()->put(data+num, size-num);
        }
        
        while(num < size){
            this->emplace_back(pool.alloc());
            num += this->back()->put(data+num, size-num);
        }
    }
    
    void append(NIOByteBuffer::unique& buf){
        this->emplace_back(std::move(buf));
    }
    
    void append(NIOByteBufferQ& other){
        while (!other.empty()) {
            this->emplace_back(std::move(other.front()));
            other.pop_front();
        }
    }
    
};


#endif /* NPool_hpp */
