//
// Created by lance on 2023/12/8.
//

#include "RingBuffer.h"
#include "PlatformSemaphore.h"
#include <cstring>
#include <atomic>

#define min(a, b)            (((a) < (b)) ? (a) : (b))

namespace morrow
{
RingBuffer::RingBuffer(uint32_t size)
{
    setDefaults();
    create(size);
}

RingBuffer::~RingBuffer()
{
    destroy();
}


void RingBuffer::create(uint32_t size)
{
    m_Reader = &m_Header.reader;
    m_Writer = &m_Header.writer;
    if (size != 0)
        m_Buffer = new char[size];
    m_BufferSize = size;
    m_Reader->reset();
    m_Writer->reset();
    m_Writer->bufferEnd = size;

    m_ReadSemaphore = new Semaphore;
    m_WriteSemaphore = new Semaphore;
}


void RingBuffer::destroy()
{
    if (m_Buffer == nullptr) return;
    delete m_Buffer;
    m_Reader->reset();
    m_Writer->reset();

    delete m_ReadSemaphore;
    delete m_WriteSemaphore;

    setDefaults();
}


void RingBuffer::readStreamingData(void* data, uint32_t size, uint32_t alignment, uint32_t step)
{
    // This should not be uint32_t, as the Device may run across processes of different
    // bitness, and the data serialized in the command buffer must match.
    uint32_t sz = readValueType<uint64_t>();

    char* dest = (char*) data;
    for (uint32_t offset = 0; offset < size; offset += step) {
        uint32_t bytes = min(size - offset, step);
        const void* src = getReadDataPointer(bytes, alignment);
        if (data)
            memcpy(dest, src, bytes);

        int32_t magic = readValueType<int32_t>();

        readReleaseData();
        dest += step;
    }
}

void RingBuffer::readReleaseData()
{
    if (m_Reader->checkedWraps == m_Reader->bufferWraps) {
        // We only update the position
        m_Reader->checkedPos = m_Reader->bufferPos;
    } else {
        std::unique_lock<std::mutex> lk(m_Mutex);
        m_Reader->checkedPos = m_Reader->bufferPos;
        m_Reader->checkedWraps = m_Reader->bufferWraps;
    }
    sendReadSignal();
}

void RingBuffer::writeStreamingData(const void* data, uint32_t size, uint32_t alignment, uint32_t step)
{
    // This should not be uint32_t, as the Device may run across processes of different
    // bitness, and the data serialized in the command buffer must match.
    writeValueType<uint64_t>(size);

    const char* src = (const char*) data;
    for (uint32_t offset = 0; offset < size; offset += step) {
        uint32_t bytes = min(size - offset, step);
        void* dest = getWriteDataPointer(bytes, alignment);
        memcpy(dest, src, bytes);
        writeValueType<int32_t>(1234);

        // In the NaCl Web Player, make sure that only complete commands are submitted, as we are not truely
        // asynchronous.

        writeSubmitData();

        src += step;
    }
    writeSubmitData();
}

void RingBuffer::writeSubmitData()
{
    if (m_Writer->checkedWraps == m_Writer->bufferWraps) {
        // We only update the position
        m_Writer->checkedPos = m_Writer->bufferPos;
    } else {
        std::unique_lock<std::mutex> lk(m_Mutex);
        m_Writer->checkedPos = m_Writer->bufferPos;
        m_Writer->checkedWraps = m_Writer->bufferWraps;
    }
    sendWriteSignal();
}

void RingBuffer::setDefaults()
{
    m_Buffer = nullptr;
    m_BufferSize = 0;
    m_ReadSemaphore = nullptr;
    m_WriteSemaphore = nullptr;
    m_NeedsReadSignal = 0;
    m_NeedsWriteSignal = 0;
};

void RingBuffer::handleReadOverflow(uint32_t& dataPos, uint32_t& dataEnd)
{
    std::unique_lock<std::mutex> lk(m_Mutex);

    if (dataEnd > m_BufferSize) {
        dataEnd -= dataPos;
        dataPos = 0;
        m_Reader->bufferPos = 0;
        m_Reader->bufferWraps++;
    }

    for (;;) {
        // Get how many buffer lengths writer is ahead of reader
        // This may be -1 if we are waiting for the writer to wrap
        uint32_t comparedPos = m_Writer->checkedPos;
        uint32_t comparedWraps = m_Writer->checkedWraps;
        uint32_t wrapDist = comparedWraps - m_Reader->bufferWraps;
        m_Reader->bufferEnd = (wrapDist == 0) ? comparedPos : (wrapDist == 1) ? m_BufferSize : 0;

        if (dataEnd <= m_Reader->bufferEnd) {
            break;
        }
        m_NeedsWriteSignal++;
        m_Mutex.unlock();
        if (comparedPos != m_Writer->checkedPos || comparedWraps != m_Writer->checkedWraps) {
            // Writer position changed while we requested a signal
            // Request might be missed, so we signal ourselves to avoid deadlock
            sendWriteSignal();
        }
        sendReadSignal();
        // Wait for writer thread
        m_WriteSemaphore->waitForSignal();
        m_Mutex.lock();
    }
}

void RingBuffer::handleWriteOverflow(uint32_t& dataPos, uint32_t& dataEnd)
{
    std::unique_lock<std::mutex> lk(m_Mutex);

    if (dataEnd > m_BufferSize) {
        dataEnd -= dataPos;
        dataPos = 0;
        m_Writer->bufferPos = 0;
        m_Writer->bufferWraps++;
    }

    for (;;) {
        // Get how many buffer lengths writer is ahead of reader
        // This may be 2 if we are waiting for the reader to wrap
        uint32_t comparedPos = m_Reader->checkedPos;
        uint32_t comparedWraps = m_Reader->checkedWraps;
        uint32_t wrapDist = m_Writer->bufferWraps - comparedWraps;
        m_Writer->bufferEnd = (wrapDist == 0) ? m_BufferSize : (wrapDist == 1) ? comparedPos : 0;

        if (dataEnd <= m_Writer->bufferEnd) {
            break;
        }
        m_NeedsReadSignal++;
        m_Mutex.unlock();
        if (comparedPos != m_Reader->checkedPos || comparedWraps != m_Reader->checkedWraps) {
            // Reader position changed while we requested a signal
            // Request might be missed, so we signal ourselves to avoid deadlock
            sendReadSignal();
        }
        sendWriteSignal();
        // Wait for reader thread
        m_ReadSemaphore->waitForSignal();
        m_Mutex.lock();
    }
}

void RingBuffer::sendReadSignal()
{
    int32_t expected = 1;
    if (m_NeedsReadSignal.compare_exchange_strong(expected, 0)) {
        m_ReadSemaphore->signal();
    }
}

void RingBuffer::sendWriteSignal()
{
    int32_t expected = 1;
    if (m_NeedsWriteSignal.compare_exchange_strong( expected, 0)) {
        m_WriteSemaphore->signal();
    }
}

uint32_t RingBuffer::align(uint32_t pos, uint32_t alignment) const
{
    return (pos + alignment - 1) & ~(alignment - 1);
}

void RingBuffer::BufferState::reset() volatile
{
    bufferPos = 0;
    bufferEnd = 0;
    bufferWraps = 0;
    checkedPos = 0;
    checkedWraps = 0;
}

}