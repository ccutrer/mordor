#ifndef __MORDOR_TEST_STREAM_H__
#define __MORDOR_TEST_STREAM_H__
// Copyright (c) 2009 - Mozy, Inc.

#include "filter.h"

namespace Mordor {

// This stream is for use in unit tests to force
class TestStream : public FilterStream
{
public:
    typedef std::shared_ptr<TestStream> ptr;

public:
    TestStream(Stream::ptr parent)
        : FilterStream(parent, true),
          m_maxReadSize(~0),
          m_maxWriteSize(~0),
          m_onReadBytes(0),
          m_onWriteBytes(0)
    {}

    size_t maxReadSize() const { return m_maxReadSize; }
    void maxReadSize(size_t max) { m_maxReadSize = max; }

    size_t maxWriteSize() const { return m_maxWriteSize; }
    void maxWriteSize(size_t max) { m_maxWriteSize = max; }

    void onClose(std::function<void (CloseType)> dg)
    { m_onClose = dg; }
    void onRead(std::function<void ()> dg, long long bytes = 0)
    { m_onRead = dg; m_onReadBytes = bytes; }
    void onWrite(std::function<void ()> dg, long long bytes = 0)
    { m_onWrite = dg; m_onWriteBytes = bytes; }
    void onFlush(std::function<void (bool)> dg)
    { m_onFlush = dg; }

    void close(CloseType type = BOTH);
    using FilterStream::read;
    size_t read(Buffer &b, size_t len);
    using FilterStream::write;
    size_t write(const Buffer &b, size_t len);
    void flush(bool flushParent = true);

private:
    size_t m_maxReadSize, m_maxWriteSize;
    std::function<void (CloseType)> m_onClose;
    std::function<void ()> m_onRead, m_onWrite;
    std::function<void (bool)> m_onFlush;
    long long m_onReadBytes, m_onWriteBytes;
};

}

#endif
