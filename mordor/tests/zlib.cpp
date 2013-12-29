#include "mordor/pch.h"

#include "mordor/exception.h"
#include "mordor/streams/zlib.h"
#include "mordor/streams/gzip.h"
#include "mordor/streams/deflate.h"
#include "mordor/streams/memory.h"
#include "mordor/streams/singleplex.h"
#include "mordor/test/test.h"

using namespace Mordor;
using namespace Mordor::Test;

static const unsigned char test_uncompressed[] = {
0x55, 0x6e, 0x66, 0x6f, 0x72, 0x74, 0x75, 0x6e, 0x61, 0x74,
0x65, 0x6c, 0x79, 0x2c, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x75,
0x74, 0x65, 0x72, 0x73, 0x20, 0x61, 0x72, 0x65, 0x20, 0x76,
0x75, 0x6c, 0x6e, 0x65, 0x72, 0x61, 0x62, 0x6c, 0x65, 0x20,
0x74, 0x6f, 0x20, 0x68, 0x61, 0x72, 0x64, 0x20, 0x64, 0x72,
0x69, 0x76, 0x65, 0x20, 0x63, 0x72, 0x61, 0x73, 0x68, 0x65,
0x73, 0x2c, 0x20, 0x76, 0x69, 0x72, 0x75, 0x73, 0x20, 0x61,
0x74, 0x74, 0x61, 0x63, 0x6b, 0x73, 0x2c, 0x20, 0x74, 0x68,
0x65, 0x66, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6e, 0x61,
0x74, 0x75, 0x72, 0x61, 0x6c, 0x20, 0x64, 0x69, 0x73, 0x61,
0x73, 0x74, 0x65, 0x72, 0x73, 0x2c, 0x20, 0x77, 0x68, 0x69,
0x63, 0x68, 0x20, 0x63, 0x61, 0x6e, 0x20, 0x65, 0x72, 0x61,
0x73, 0x65, 0x20, 0x65, 0x76, 0x65, 0x72, 0x79, 0x74, 0x68,
0x69, 0x6e, 0x67, 0x20, 0x69, 0x6e, 0x20, 0x61, 0x6e, 0x20,
0x69, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x2e, 0x0d, 0x0a
};

static const unsigned char test_deflate[] = {
0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03, 0xd0, 0x3b,
0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20, 0xb4, 0xf9,
0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6, 0x84, 0x8b,
0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96, 0x52, 0xf0,
0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39, 0xc8, 0x18,
0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84, 0x90, 0x35,
0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b, 0x76, 0xb7,
0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c, 0x04, 0x48,
0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9, 0x1f, 0x2c,
0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d, 0xc1, 0x9b,
0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65, 0x7a, 0x90,
0xc6, 0xed, 0x7a, 0xf9, 0x01
};

static const unsigned char test_gzip[] = {
0x1f, 0x8b, 0x08, 0x08, 0x4f, 0xa7, 0xd4, 0x4a, 0x00, 0x03,
0x73, 0x6f, 0x6d, 0x65, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x00,
0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03, 0xd0, 0x3b,
0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20, 0xb4, 0xf9,
0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6, 0x84, 0x8b,
0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96, 0x52, 0xf0,
0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39, 0xc8, 0x18,
0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84, 0x90, 0x35,
0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b, 0x76, 0xb7,
0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c, 0x04, 0x48,
0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9, 0x1f, 0x2c,
0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d, 0xc1, 0x9b,
0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65, 0x7a, 0x90,
0xc6, 0xed, 0x7a, 0xf9, 0x01, 0x32, 0x81, 0xa0, 0xbb, 0x96,
0x00, 0x00, 0x00
};

static const unsigned char test_zlib[] = {
0x78, 0x9c, 0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03,
0xd0, 0x3b, 0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20,
0xb4, 0xf9, 0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6,
0x84, 0x8b, 0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96,
0x52, 0xf0, 0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39,
0xc8, 0x18, 0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84,
0x90, 0x35, 0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b,
0x76, 0xb7, 0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c,
0x04, 0x48, 0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9,
0x1f, 0x2c, 0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d,
0xc1, 0x9b, 0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65,
0x7a, 0x90, 0xc6, 0xed, 0x7a, 0xf9, 0x01, 0xaa, 0x68, 0x37,
0x41
};

// for decompression, we will take the pre-compressed test data for the compression method,
// decompress it, and compare it to the original data
template <class StreamType>
void testDecompress(const unsigned char *test_data, size_t size)
{
    Buffer controlBuf;
    controlBuf.copyIn(test_uncompressed, sizeof(test_uncompressed));

    Buffer compressed;
    compressed.copyIn(test_data, size);
    Stream::ptr memstream(new MemoryStream(compressed));
    Stream::ptr readplex(new SingleplexStream(memstream, SingleplexStream::READ));
    StreamType teststream(readplex);
    Buffer testBuf;
    while(0 < teststream.read(testBuf, 4096));

    MORDOR_TEST_ASSERT( controlBuf == testBuf );
}

// for compression, we will compress some data, decompress it, and compare to the original.
// we can't just compare to the existing compressed data test, because there is more than one
// valid compressed representation of a file--but we verified decompression works above.
template <class StreamType>
void testCompress()
{
    Buffer origData;
    origData.copyIn(test_uncompressed, sizeof(test_uncompressed));

    // compress from origData to compressed
    std::shared_ptr<MemoryStream> memstream(new MemoryStream());
    Stream::ptr writeplex(new SingleplexStream(memstream, SingleplexStream::WRITE));
    StreamType teststream(writeplex);
    teststream.write(origData, origData.readAvailable());
    teststream.close();

    // decompress from compressed to decomp
    Buffer decomp;
    Stream::ptr memstream2(new MemoryStream(memstream->buffer()));
    Stream::ptr readplex(new SingleplexStream(memstream2, SingleplexStream::READ));
    StreamType teststream2(readplex);
    while(0 < teststream2.read(decomp, 4096));

    // compare origData to decomp
    MORDOR_TEST_ASSERT( origData == decomp );
}


MORDOR_UNITTEST(ZlibStream, compress)
{
    testCompress<ZlibStream>();
}

MORDOR_UNITTEST(ZlibStream, decompress)
{
    testDecompress<ZlibStream>(test_zlib, sizeof(test_zlib));
}

MORDOR_UNITTEST(GzipStream, compress)
{
    testCompress<GzipStream>();
}

MORDOR_UNITTEST(GzipStream, decompress)
{
    testDecompress<GzipStream>(test_gzip, sizeof(test_gzip));
}

MORDOR_UNITTEST(DeflateStream, compress)
{
    testCompress<DeflateStream>();
}

MORDOR_UNITTEST(DeflateStream, decompress)
{
    testDecompress<DeflateStream>(test_deflate, sizeof(test_deflate));
}

