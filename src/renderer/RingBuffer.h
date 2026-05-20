//
// Created by lance on 2023/12/8.
//

#ifndef MORROW_RENDERER_RINGBUFFER_H_
#define MORROW_RENDERER_RINGBUFFER_H_

#include <new> // for placement new
#include <mutex>
#include <atomic>
#include <memory>

#define ALIGN_OF(T) __alignof__(T)

namespace morrow
{
class Semaphore;

class RingBuffer
{
public:
    enum
    {
        kDefaultAlignment = 4,
        kDefaultStep = 2048
    };

    struct BufferState
    {
        // These should not be uint32_t, as the Device may run across processes of different
        // bitness, and the data serialized in the command buffer must match.
        void reset() volatile;

        volatile uint64_t bufferPos;
        volatile uint64_t bufferEnd;
        volatile uint64_t bufferWraps;
        volatile uint64_t checkedPos;
        volatile uint64_t checkedWraps;
    };

    struct BufferHeader
    {
        BufferState reader;
        BufferState writer;
    };


    RingBuffer(uint32_t size);

    ~RingBuffer();

    // Read data from the ringbuffer
    // This function blocks until data new data has arrived in the ringbuffer.
    // It uses semaphores to wait on the producer thread in a efficient way.
    template<class T>
    const T& readValueType();

    // ReadReleaseData should be called when the data has been read & used completely.
    // At this point the memory will become available to the producer to write into it again.
    void readReleaseData();

    // Write data into the ringbuffer
    template<class T>
    void writeValueType(const T& val);

    // WriteSubmitData should be called after data has been completely written and should be made available to the consumer thread to read it.
    // Before WriteSubmitData is called, any data written with WriteValueType can not be read by the consumer.
    void writeSubmitData();

    // Ringbuffer Streaming support. This will automatically call WriteSubmitData & ReadReleaseData.
    // It splits the data into smaller chunks (step). So that the size of the ringbuffer can be smaller than the data size passed into this function.
    // The consumer thread will be reading the streaming data while WriteStreamingData is still called on the producer thread.
    void readStreamingData(void* data, uint32_t size, uint32_t alignment = kDefaultAlignment, uint32_t step = kDefaultStep);

    void writeStreamingData(const void* data, uint32_t size, uint32_t alignment = kDefaultAlignment, uint32_t step = kDefaultStep);


    // Utility functions
    void* getReadDataPointer(uint32_t size, uint32_t alignment);

    void* getWriteDataPointer(uint32_t size, uint32_t alignment);


    // Creation methods
    void create(uint32_t size);

    void destroy();


private:
    uint32_t align(uint32_t pos, uint32_t alignment) const;

    void setDefaults();

    void handleReadOverflow(uint32_t& dataPos, uint32_t& dataEnd);

    void handleWriteOverflow(uint32_t& dataPos, uint32_t& dataEnd);

    void sendReadSignal();

    void sendWriteSignal();

    char* m_Buffer;
    uint32_t m_BufferSize;
    BufferState* m_Reader;
    BufferState* m_Writer;
    BufferHeader m_Header;
    std::mutex m_Mutex;
    Semaphore* m_ReadSemaphore;
    Semaphore* m_WriteSemaphore;
    std::atomic_int m_NeedsReadSignal;
    std::atomic_int m_NeedsWriteSignal;
};


inline void* RingBuffer::getReadDataPointer(uint32_t size, uint32_t alignment)
{
    size = align(size, alignment);
    uint32_t dataPos = align(m_Reader->bufferPos, alignment);
    uint32_t dataEnd = dataPos + size;
    if (dataEnd > m_Reader->bufferEnd) {
        handleReadOverflow(dataPos, dataEnd);
    }
    m_Reader->bufferPos = dataEnd;

    return &m_Buffer[dataPos];
}

inline void* RingBuffer::getWriteDataPointer(uint32_t size, uint32_t alignment)
{
    size = align(size, alignment);
    uint32_t dataPos = align(m_Writer->bufferPos, alignment);
    uint32_t dataEnd = dataPos + size;
    if (dataEnd > m_Writer->bufferEnd) {
        handleWriteOverflow(dataPos, dataEnd);
    }
    m_Writer->bufferPos = dataEnd;

    return &m_Buffer[dataPos];
}

template<class T>
inline const T& RingBuffer::readValueType()
{
    // Read simple data type from queue
    const void* pdata = getReadDataPointer(sizeof(T), ALIGN_OF(T));
    const T& src = *reinterpret_cast<const T*>(pdata);
    return src;
}

template<class T>
inline void RingBuffer::writeValueType(const T& val)
{
    // Write simple data type to queue
    void* pdata = getWriteDataPointer(sizeof(T), ALIGN_OF(T));
    new(pdata) T(val);
}

using RingBufferSharedPtr = std::shared_ptr<RingBuffer>;
} // MORROWGUI

#endif //MORROW_RENDERER_RINGBUFFER_H_
